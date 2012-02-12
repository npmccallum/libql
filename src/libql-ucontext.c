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

#include "libql-internal.h"
#include <assert.h>
#include <string.h>
#include <ucontext.h>

#define MINPAGES 4
#define INTPERPOINTER 4 /* This should work even on a 128bit system. */

typedef struct qlStateUContext {
	qlState      state;
	volatile int jumped;
	ucontext_t   stpctx;
	ucontext_t   yldctx;
} qlStateUContext;

size_t
eng_ucontext_size()
{
	assert(MINPAGES > 1);
	assert(sizeof(qlStateUContext) < get_pagesize());
	return MINPAGES * get_pagesize();
}

void
eng_ucontext_new(qlStateUContext *state)
{
	size_t pagesize = get_pagesize();

	/* Assure that state was allocated sizeof(qlStateShift) below a page */
	assert(((uintptr_t) state) / pagesize ==
          (((uintptr_t) state) + sizeof(qlStateUContext)) / pagesize);

	/* Assure that a pointer will fit in INTPERPOINTER integers */
	assert(sizeof(void*) < (sizeof(int) * INTPERPOINTER));
}

static void
inside_context(int a, int b, int c, int d)
{
	qlState **state;
	qlParameter param;
	int args[INTPERPOINTER] = {a, b, c, d};

	/* Decode the state pointer. */
	memcpy(&state, &args[0], sizeof(qlState**));

	/* Execute the function */
	param = (*state)->func(state, *(*state)->param);
	if ((*state)->param)
		*(*state)->param = param;

	/* Jump back */
	((qlStateUContext*) *state)->jumped = -1;
}

int
eng_ucontext_step(qlStateUContext **state, qlParameter *param)
{
	qlParameter *ptmp;
	size_t pagesize;

	pagesize = get_pagesize();

	(*state)->jumped = 0;
	if (getcontext(&(*state)->stpctx) != 0)
		return STATUS_ERROR;
	if ((*state)->jumped > 0)
		return STATUS_OK;
	else if ((*state)->jumped < 0) {
		(*state)->state.free((*state)->state.ctx, *state, (*state)->state.size);
		*state = NULL;
		return STATUS_OK;
	}

	if ((*state)->state.func) {
		int args[INTPERPOINTER];
		memset(args, 0, sizeof(args));

		if (getcontext(&(*state)->yldctx) != 0)
			return STATUS_ERROR;
		(*state)->yldctx.uc_link = &(*state)->stpctx;
		(*state)->yldctx.uc_stack.ss_sp = (void*) ((((uintptr_t) &(*state)[1])
                                             / pagesize + 1) * pagesize);
		(*state)->yldctx.uc_stack.ss_size = (((*state)->state.size / pagesize - 1)
				                             * pagesize);

		/* Encode the pointer into the args buffer and execute. */
		memcpy(&args[0], &state, sizeof(qlState**));
		makecontext(&(*state)->yldctx, (void (*)(void)) inside_context,
				    INTPERPOINTER, args[0], args[1], args[2], args[3]);
	}

	if (param)
		*(*state)->state.param = *param;
	ptmp = (*state)->state.param;
	(*state)->state.param = param;

	(*state)->jumped = 1;
	if (setcontext(&(*state)->yldctx) != 0) {
		(*state)->state.param = ptmp;
		return STATUS_ERROR;
	}
	assert(0); /* Never get here */
	return 0;
}

int
eng_ucontext_yield(qlStateUContext **state, qlParameter *param)
{

	(*state)->jumped = 0;
	if (getcontext(&(*state)->yldctx) != 0)
		return STATUS_ERROR;
	if ((*state)->jumped) {
		if (!(*state)->state.param)
			return STATUS_CANCEL;
		return STATUS_OK;
	}

	(*state)->jumped = 1;
	if (setcontext(&(*state)->stpctx) != 0)
		return STATUS_ERROR;

	assert(0); /* Never get here */
	return STATUS_ERROR;
}

void
eng_ucontext_cancel(qlStateUContext **state)
{

}
