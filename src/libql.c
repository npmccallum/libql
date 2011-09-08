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
	void    **misc;
	jmp_buf   srcbuf;
	jmp_buf   dstbuf;
	void     *srcpos;
	void     *dstpos;
	void     *dststk;
};

int
ql_call(qlFunction *call, qlState **state, void** misc)
{
	int retval = 0;

	if (!call || !state)
		return 0;

	if (!*state) {
		if (!misc) /* Don't permit null misc on first call */
			return 0;

		/* Allocate a buffer big enough for our state switches */
		if (!(*state = malloc(sizeof(qlState))))
			return 0;

		(*state)->misc   = misc;    /* Setup misc storage */
		(*state)->srcpos = &retval; /* Mark the stack we will jump from */
		(*state)->dststk = NULL;    /* We have no stack on the heap */
	}

	/* Store the current state */
	retval = setjmp((*state)->srcbuf);
	if (retval != 0)
		return 1; /* We are resuming from ql_yield() */

	if ((*state)->dststk) {
		/* Ensure we are restoring at the same point in the stack */
		if ((*state)->srcpos != &retval)
			return 0;

		/* Restore the memory */
		alloca(DIFF((*state)->srcpos, (*state)->dstpos)); /* Push the stack */
		memcpy(START((*state)->srcpos, (*state)->dstpos), (*state)->dststk,
				DIFF((*state)->srcpos, (*state)->dstpos));

		/* Free the stack we just loaded */
		free((*state)->dststk);
		(*state)->dststk = NULL;

		/* Set the misc for the resume */
		if (misc)
			*(*state)->misc = *misc;
		(*state)->misc = misc;

		longjmp((*state)->dstbuf, misc ? 1 : -1);
		return 0; /* We will never get here */
	}

	*misc = (*call)(*state, *misc);
	free(*state);
	*state = NULL;

	return 1;
}

int
ql_yield(qlState *state, void **misc)
{
	int retval = 0;

	if (!state || !misc || !state->misc)
		return 0;

	/* Store our state */
	retval = setjmp(state->dstbuf);
	if (retval != 0)
		return retval;

	/* Mark the high watermark of the stack to save */
	state->dstpos = &retval;

	/* Allocate the buffer to store the stack in */
	state->dststk = malloc(DIFF(state->srcpos, state->dstpos));
	if (!state->dststk)
		return 0;

	/* Pass our misc data back */
	*state->misc = *misc;
	state->misc = misc;

	/* Copy stack into the heap and jump */
	memcpy(state->dststk, START(state->srcpos, state->dstpos),
						  DIFF(state->srcpos, state->dstpos));
	longjmp(state->srcbuf, 1);
	return 0; /* We will never get here */
}

