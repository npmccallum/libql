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

#include <libql.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t int_ql_size(void);
size_t int_ql_current_partial_stack_size(qlState *state);

int    int_ql_call(qlFunction *call, qlState *state, void** misc);
int    int_ql_yield(qlState *state);

struct qlState {
	void **misc;
	void *stack;
} __attribute__((__packed__));

int
ql_call(qlFunction *call, qlState **state, void** misc)
{
	size_t size = 0;
	int retval = 0;

	if (!call || !state)
		return 0;

	if (!*state) {
		if (!misc) /* Don't permit null misc on first call */
			return 0;

		/* Allocate a buffer big enough for our state switches */
		size = int_ql_size();
		if (!(*state = malloc(size)))
			return 0;
		memset(*state, 0, size);
	}

	retval = int_ql_call(call, *state, misc);
	if (retval > 1) {
		free(*state);
		*state = NULL;
	}

	return retval > 0;
}

int
ql_yield(qlState *state, void **misc)
{
	size_t size = 0;
	int retval = 0;

	if (!state || !misc || !state->misc)
		return 0;

	size = int_ql_current_partial_stack_size(state);
	state->stack = malloc(size);
	if (!state->stack)
		return 0;
	*state->misc = *misc;
	state->misc = misc;

	retval = int_ql_yield(state);
	free(state->stack);
	state->stack = NULL;
	if (!state->misc)
		return -1;
	return retval > 0;
}

