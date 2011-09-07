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
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#define DOUBLE(n) n = (void*) (((intptr_t) n) * 2)

static void *
level1(qlState *state, void **misc)
{
	DOUBLE(*misc);
	printf("level1-1: %p\n", *misc);
	DOUBLE(*misc);
	ql_yield(state, misc);
	printf("level1-2: %p\n", *misc);
	return DOUBLE(*misc);
}

static void *
level0(qlState *state, void *misc)
{
	DOUBLE(misc);
	printf("level0-1: %p\n", misc);
	DOUBLE(misc);
	assert(ql_yield(state, &misc) > 0);
	printf("level0-2: %p\n", misc);

	misc = level1(state, &misc);
	printf("level0-3: %p\n", misc);
	return DOUBLE(misc);
}

int main() {
	qlState *state = NULL;
	void *misc = (void*) 0x1;

	do {
		assert(ql_call(level0, &state, &misc));
		if (state)
			printf("yielded: %p\n", misc);
		else
			printf("returned: %p\n", misc);
		DOUBLE(misc);
	} while (state);

	return 0;
}
