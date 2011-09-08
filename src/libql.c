/*
 * libql - A coroutines library for C/C++
 *
 * Copyright 2011 Nathaniel McCallum <nathaniel@themccallums.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libql.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
#include <alloca.h>

#define DIFF(src, dst)  (labs(dst - src))
#define START(one, two) ((one) < (two) ? (one) : (two))

struct qlState {
	/* Setup in ql_init() */
	qlFunction *func;
	qlResize *resize;
	qlFree	   *free;
	void        *ctx;
	size_t      size;

	/* Setup in ql_call()/ql_yield() */
	qlState      **ref;
	qlParameter* param;
	jmp_buf     srcbuf;
	jmp_buf     dstbuf;
	void       *srcpos;
	void       *dstpos;
	int         resume;
};

static void *
int_resize(void *ctx, void *mem, size_t size)
{
	return realloc(mem, size);
}

static void
int_free(void *ctx, void *mem, size_t size)
{
	free(mem);
}

qlState *
ql_state_init(qlFunction *func)
{
	return ql_state_init_size(func, 0);
}

qlState *
ql_state_init_size(qlFunction *func, size_t size)
{
	return ql_state_init_full(func, size, NULL, int_resize, int_free, NULL);
}

qlState *
ql_state_init_full(qlFunction *func, size_t size, void *memory,
		     qlResize *resize, qlFree *free, void *ctx)
{
	qlState *state = memory;

	if (!func)
		return NULL;
	if (!resize && !memory)
		resize = int_resize;
	if (!free && !memory)
		free = int_free;

	if (!memory || size < sizeof(qlState)) {
		if (!resize)
			return NULL;
		size = memory ? sizeof(qlState) : (size + sizeof(qlState));
		qlState *tmp = (*resize)(ctx, state, size);
		if (!tmp)
			return NULL;
		state = tmp;
	}

	memset(state, 0, sizeof(qlState));
	state->func   = func;
	state->resize = resize;
	state->free   = free;
	state->ctx    = ctx;
	state->size   = size;
	return state;
}

int
ql_state_step(qlState **state, qlParameter* param)
{
	int retval = 0;

	if (!state || !*state)
		return 0;

	/* This is where we will store a reference if the state is reallocated */
	(*state)->ref = state;

	/* Store the current state */
	retval = setjmp((*state)->srcbuf);
	if (retval != 0)
		return 1; /* We are resuming from ql_yield() */

	if ((*state)->resume) {
		/* Ensure we are restoring at the same point in the stack */
		if ((*state)->srcpos != &retval)
			return 0;

		/* Restore the memory */
		alloca(DIFF((*state)->srcpos, (*state)->dstpos)); /* Push the stack */
		memcpy(START((*state)->srcpos, (*state)->dstpos), &(*state)[1],
				DIFF((*state)->srcpos, (*state)->dstpos));

		/* Set the param for the resume */
		if (param)
			*(*state)->param = *param;
		(*state)->param = param;
		(*state)->resume = 0; /* We've resumed */

		longjmp((*state)->dstbuf, param ? 1 : -1);
		return 0; /* We will never get here */
	}

	/* Ensure we have param on the first call (otherwise yield() won't work) */
	if (!param)
		return 0;
	(*state)->param = param;

	(*state)->srcpos = &retval; /* Mark the stack we will jump from */
	*param = (*state)->func(*state, *param);
	(*state)->free((*state)->ctx, *state, (*state)->size);
	*state = NULL;

	return 1;
}

static int
resize_fail(qlState *state)
{
	qlState *tmp = NULL;
	size_t needed = sizeof(qlState) + DIFF(state->srcpos, state->dstpos);

	if (state->size < needed) {
		if (!state->resize)
			return 1;
		tmp = state->resize(state->ctx, state, needed);
		if (!tmp)
			return 1;
		*state->ref = tmp;
	}
	return 0;
}

int
ql_state_yield(qlState *state, qlParameter* param)
{
	int retval = 0;

	if (!state || !param || !state->param)
		return 0;

	/* Store our state */
	retval = setjmp(state->dstbuf);
	if (retval != 0)
		return retval;

	/* Mark the high watermark of the stack to save */
	state->dstpos = &retval;

	/* Reallocate the buffer if need be */
	if (resize_fail(state))
		return 0;

	/* Pass our misc data back */
	*state->param = *param;
	state->param = param;
	state->resume = 1;

	/* Copy stack into the heap and jump */
	memcpy(&state[1], START(state->srcpos, state->dstpos),
                      DIFF(state->srcpos, state->dstpos));
	longjmp(state->srcbuf, 1);
	return 0; /* We will never get here */
}

