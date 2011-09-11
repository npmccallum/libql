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
#include <libqlsp.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <assert.h>
#include <string.h>

#define START 25
#define YIELDS 5
#define END (START * 1000 * 1000)

#define FMT "%s,%u,%d.%06u,%u\n"
#define DIFF(one, two) ((one > two) ? (one - two) : (two - one))
#define SECONDS(stv, etv) ((unsigned int) (etv.tv_sec - stv.tv_sec - \
	                       ((stv.tv_usec > etv.tv_usec) ? 1 : 0)))
#define USECONDS(stv, etv) ((unsigned int) DIFF(etv.tv_usec, stv.tv_usec))
#define TIME(stv, etv) SECONDS(stv, etv), USECONDS(stv, etv)

void *(*int_malloc)(size_t size);
void *(*int_realloc)(void *mem, size_t size);
void *(*int_calloc)(size_t count, size_t size);

qlParameter
test_return(qlState **state, qlParameter param)
{
	return param;
}

qlParameter
test_yield(qlState **state, qlParameter param)
{
	int i;
	for (i=0; i < YIELDS - 1; i++)
		ql_state_yield(state, &param);
	return param;
}

int
main(int argc, char **argv)
{
	struct timeval stv, etv;
	qlState *state;
	qlStatePool *pool;
	qlParameter param;
	unsigned int i;

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		for (param.uint32=0; param.uint32 < i; param.uint32++)
			param = test_return(NULL, param);
		gettimeofday(&etv, NULL);
		printf(FMT, "return", i, TIME(stv, etv), 0);
	}

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		for (param.uint32=0; param.uint32 < i / YIELDS; param.uint32++) {
			state = ql_state_init(test_yield);
			while (state)
				ql_state_step(&state, &param);
		}
		gettimeofday(&etv, NULL);
		printf(FMT, "yield", i, TIME(stv, etv), i / YIELDS * 2);
	}

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		for (param.uint32=0; param.uint32 < i / YIELDS; param.uint32++) {
			state = ql_state_init_size(test_yield, 1024);
			while (state)
				ql_state_step(&state, &param);
		}
		gettimeofday(&etv, NULL);
		printf(FMT, "prealloc", i, TIME(stv, etv), i / YIELDS);
	}

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		pool = ql_state_pool_init(5);
		for (param.uint32=0; param.uint32 < i / YIELDS; param.uint32++) {
			state = ql_state_pool_state_init(pool, test_yield, 0);
			while (state)
				ql_state_step(&state, &param);
		}
		ql_state_pool_free(pool);
		gettimeofday(&etv, NULL);
		printf(FMT, "pooled", i, TIME(stv, etv), 2);
	}

	return 0;
}
