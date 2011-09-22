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

typedef struct qlStateShift {
	qlState      state;
	volatile int jumped;
	ucontext_t   stpctx;
	ucontext_t   yldctx;
} qlStateShift;

size_t
ucontext_size()
{
	assert(MINPAGES > 1);
	assert(sizeof(qlStateShift) < get_pagesize());
	return MINPAGES * get_pagesize();
}

void
ucontext_init(qlState *state)
{
	size_t pagesize = get_pagesize();

	/* Assure that state was allocated sizeof(qlStateShift) below a page */
	assert(((uintptr_t) state) / pagesize ==
          (((uintptr_t) state) + sizeof(qlStateShift)) / pagesize);

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
	((qlStateShift*) *state)->jumped = -1;
}

int
ucontext_step(qlState **state, qlParameter *param)
{
	qlStateShift *states = (qlStateShift*) *state;
	size_t pagesize;

	pagesize = get_pagesize();

	states->jumped = 0;
	getcontext(&states->stpctx);
	if (states->jumped > 0)
		return 0;
	else if (states->jumped < 0) {
		(*state)->free((*state)->ctx, *state, (*state)->size);
		*state = NULL;
		return 0;
	}

	if (states->state.func) {
		int args[INTPERPOINTER];

		getcontext(&states->yldctx);
		states->yldctx.uc_link = &states->stpctx;
		states->yldctx.uc_stack.ss_sp = (void*) ((((uintptr_t) &states[1])
                                             / pagesize + 1) * pagesize);
		states->yldctx.uc_stack.ss_size = ((states->state.size / pagesize - 1)
				                             * pagesize);

		/* Encode the pointer into the args buffer and execute. */
		memcpy(&args[0], &state, sizeof(qlState**));
		makecontext(&states->yldctx, (void (*)(void)) inside_context,
				    INTPERPOINTER, args[0], args[1], args[2], args[3]);
	}

	if (param)
		*states->state.param = *param;
	states->state.param = param;

	states->jumped = 1;
	setcontext(&states->yldctx);
	assert(0); /* Never get here */
	return 0;
}

int
ucontext_yield(qlState **state, qlParameter *param)
{
	qlStateShift *states = (qlStateShift*) *state;
	qlParameter *ptmp;

	states->jumped = 0;
	if (getcontext(&states->yldctx) != 0)
		return STATUS_ERROR;
	if (states->jumped) {
		if (!(*state)->param)
			return STATUS_CANCEL;
		return STATUS_OK;
	}

	ptmp = states->state.param;
	states->state.param = param;
	states->jumped = 1;
	if (setcontext(&states->stpctx) != 0) {
		states->state.param = ptmp;
		return STATUS_ERROR;
	}
	assert(0); /* Never get here */
	return STATUS_ERROR;
}
