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
#include <assert.h>
#include <string.h>

#define START 25
#define YIELDS 5
#define END (START * 1000 * 10)

#define RFMT "%s,%u,%d.%06u,%u\n"
#define FMT "%s-%s-" RFMT
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

static void
dorun(const char *eng, qlFlags flags)
{
	struct timeval stv, etv;
	const char *method;
	qlParameter param;
	qlStatePool *pool;
	qlState *state;
	int i, j;

	if (flags & QL_FLAG_METHOD_COPY)
		method = "copy";
	else if (flags & QL_FLAG_METHOD_SHIFT)
		method = "shift";
	else {
		method = NULL; /* Make the compiler shut-up */
		assert(0);
	}

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		for (j=0; j < i / YIELDS; j++) {
			state = ql_state_new(eng, flags, test_yield, 0);
			while (state)
				ql_state_step(&state, &param);
		}
		gettimeofday(&etv, NULL);
		printf(FMT, eng, method, "yield", i,
               TIME(stv, etv), i / YIELDS * 2);
	}

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		pool = ql_state_pool_new(5);
		for (j=0; j < i / YIELDS; j++) {
			state = ql_state_pool_state_new(pool, eng, flags,
                                             test_yield, 0);
			while (state)
				ql_state_step(&state, &param);
		}
		ql_state_pool_free(pool);
		gettimeofday(&etv, NULL);
		printf(FMT, eng, method, "pooled", i,
               TIME(stv, etv), 2);
	}
}

int
main(int argc, char **argv)
{
	struct timeval stv, etv;
	unsigned int i, j;
	const char * const *engines;
	qlFlags methods[] = {
		QL_FLAG_METHOD_COPY,
		QL_FLAG_METHOD_SHIFT,
		QL_FLAG_NONE
	};

	for (i=START ; i < END+1 ; i *= 10) {
		gettimeofday(&stv, NULL);
		for (j=0; j < i; j++)
			i += (uintptr_t) test_return(NULL, NULL);
		gettimeofday(&etv, NULL);
		printf(RFMT, "return", i, TIME(stv, etv), 0);
	}

	engines = ql_engine_list();
	assert(engines);

	for (i=0; engines[i]; i++) {
		qlFlags flags = ql_engine_get_flags(engines[i]);

		for (j=0; methods[j] != QL_FLAG_NONE; j++) {
			if (!(flags & methods[j]))
				continue;
			dorun(engines[i], methods[j]);
		}
	}

	return 0;
}
