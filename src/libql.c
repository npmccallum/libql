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
#include <assert.h>

#ifdef _WIN32
#define CCONV __cdecl
#else
#define CCONV
#endif

int CCONV
get_stack_direction(void);

uintptr_t CCONV
get_stack(void);

qlParameter * CCONV
translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);

void CCONV
resume_function(jmp_buf dstbuf, jmp_buf srcbuf, uintptr_t dst,
                uintptr_t src, void *buf, qlParameter *param);

void CCONV
call_function(qlState **state, qlParameter *param, qlFunction *func,
              uintptr_t stack, jmp_buf srcbuf);


#define DIFF(src, dst)  (labs(dst - src))
#define START(one, two) ((one) < (two) ? (one) : (two))
#define MINSTACK(method) (method == QL_METHOD_SHIFT ? 16384 : sizeof(qlState))
#define ALIGN(m) (m / 16 * 16)

#define JUMPBUFF 4096

#define FLAG_RESUME 2

struct qlState {
	/* Setup in ql_init() */
	qlFunction *func;
	qlResize *resize;
	qlFree	   *free;
	void        *ctx;
	size_t      size;

	/* Setup in ql_call()/ql_yield() */
	qlParameter* param;
	jmp_buf     srcbuf;
	jmp_buf     dstbuf;
	uintptr_t   srcpos;
	uintptr_t   dstpos;
	uint8_t      flags;
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

/* This little helper function exists because some platforms put
 * longjmp in the PLT and I'd rather not write code to detect it. */
void
dolongjmp(jmp_buf state, int value)
{
	longjmp(state, value);
}

qlState *
ql_state_init(qlMethod method, qlFunction *func, size_t size)
{
	return ql_state_init_full(method, func, size, NULL, NULL, NULL, NULL);
}

qlState *
ql_state_init_full(qlMethod method, qlFunction *func, size_t size,
                   void *memory, qlResize *resize, qlFree *free, void *ctx)
{
	qlState *state;

	if (!func)
		return NULL;
	if (!resize && !memory)
		resize = int_resize;
	if (!free && !memory)
		free = int_free;

	/* Make sure we at least have our minimum stack */
	if (!memory || ALIGN(size) < MINSTACK(method) ||
			/* The shift method requires 16 byte alignment */
			((method & QL_METHOD_SHIFT) && size % 16 != 0)) {
		if (!resize)
			return NULL;

		size = ALIGN(size) < MINSTACK(method) ? MINSTACK(method) : ALIGN(size);
		state = (*resize)(ctx, memory, size);
		if (!state)
			return NULL;
	} else
		state = memory;

	state->func   = func;
	state->resize = resize;
	state->free   = free;
	state->ctx    = ctx;
	state->size   = size;
	state->flags  = method & 1;
	state->dstpos = 0; /* Necessary for NULL detection later on */
	return state;
}

int
ql_state_step(qlState **state, qlParameter* param)
{
	if (!state || !*state)
		return 0;

	/* Store the current state */
	switch (setjmp((*state)->srcbuf)) {
	case 0: /* We have marked our state. */
		break;
	case 1: /* The function has returned. */
		(*state)->free((*state)->ctx, *state, (*state)->size);
		*state = NULL;
		return 1;
	case 2: /* We are resuming from ql_state_yield(). */
		return 1;
	default:
		assert(0);
		break;
	}

	if ((*state)->flags & FLAG_RESUME) {
		/* Ensure we are restoring from lower in the stack */
		if (!((*state)->flags & QL_METHOD_SHIFT)) {
			if (get_stack_direction() > 0
					? get_stack() > (*state)->srcpos
					: get_stack() < (*state)->srcpos)
				return 0;
		}

		/* Set the parameter */
		if (param) {
			qlParameter *tmp;
			tmp = translate((*state)->dstpos, (*state)->srcpos,
					        &(*state)[1], (*state)->param);
			*tmp = *param;
		}
		(*state)->param = param;

		(*state)->flags &= ~FLAG_RESUME; /* We've resumed */
		resume_function((*state)->dstbuf, (*state)->srcbuf,
                        (*state)->dstpos, (*state)->srcpos,
                        ((*state)->flags & QL_METHOD_SHIFT)
                        	? NULL : &(*state)[1], param);
		assert(0); /* We will never get here */
	}

	/* Ensure we have param on the first call (otherwise yield() won't work) */
	if (!param)
		return 0;
	(*state)->param = param;

	/* Figure out where our execution stack will lie */
	if ((*state)->flags & QL_METHOD_SHIFT) {
		(*state)->srcpos = (uintptr_t) *state;
		if (get_stack_direction() > 0)
			(*state)->srcpos += (sizeof(qlState) / 512 + 1) * 512;
		else
			(*state)->srcpos += (*state)->size;
	} else {
		(*state)->srcpos = get_stack_direction() > 0
								? get_stack() + JUMPBUFF
								: get_stack() - JUMPBUFF;
	}

	call_function(state, param, (*state)->func,
                  (*state)->srcpos, (*state)->srcbuf);
	assert(0); /* We will never get here */
	return 0; /* Make the compiler happy */
}

int
ql_state_yield(qlState **state, qlParameter* param)
{
	size_t needed;
	int retval;

	if (!state || !*state || !param || !(*state)->param)
		return 0;

	/* Store our state */
	retval = setjmp((*state)->dstbuf);
	if (retval != 0)
		return retval;

	if (!((*state)->flags & QL_METHOD_SHIFT)) {
		/* Mark the high watermark of the stack to save */
		(*state)->dstpos = get_stack();

		/* Reallocate the buffer if need be */
		needed = sizeof(qlState) + DIFF((*state)->srcpos, (*state)->dstpos);
		if ((*state)->size < needed) {
			qlState *tmp;

			if (!(*state)->resize)
				return 0;

			tmp = (*state)->resize((*state)->ctx, *state, needed);
			if (!tmp)
				return 0;

			*state = tmp;
			(*state)->size = needed;
		}

		/* Copy stack into the heap */
		memcpy(&(*state)[1], (void*) START((*state)->srcpos, (*state)->dstpos),
		       DIFF((*state)->srcpos, (*state)->dstpos));
	}

	/* Pass our misc data back */
	*(*state)->param = *param;
	(*state)->param = param;
	(*state)->flags |= FLAG_RESUME;

	longjmp((*state)->srcbuf, 2);
	assert(0); /* We will never get here */
	return 0; /* Make the compiler happy */
}

