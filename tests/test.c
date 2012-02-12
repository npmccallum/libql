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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DOUBLE(v) v = (qlParameter) (((uintptr_t) v) * 2);
#define LASTVAL   ((qlParameter) 0x2000)

static qlParameter
level2(qlState **state, qlParameter *param)
{
	DOUBLE(*param);
	printf("\tlevel2-1: %p\n", *param);

	DOUBLE(*param);
	assert(ql_state_yield(state, param) == 0);
	printf("\tlevel2-2: %p\n", *param);

	DOUBLE(*param);
	return *param;
}

static qlParameter
level1(qlState **state, qlParameter *param)
{
	DOUBLE(*param);
	printf("\tlevel1-1: %p\n", *param);

	DOUBLE(*param);
	assert(ql_state_yield(state, param) == 0);
	printf("\tlevel1-2: %p\n", *param);

	*param = level2(state, param);
	printf("\tlevel1-3: %p\n", *param);

	DOUBLE(*param);
	return *param;
}

static qlParameter
level0(qlState **state, qlParameter param)
{
	DOUBLE(param);
	printf("\tlevel0-1: %p\n", param);
	DOUBLE(param);
	assert(ql_state_yield(state, &param) == 0);
	printf("\tlevel0-2: %p\n", param);

	param = level1(state, &param);
	printf("\tlevel0-3: %p\n", param);
	DOUBLE(param);
	return param;
}

static int
step0(qlState **state, qlParameter *param, char *x)
{
	return ql_state_step(state, param);
}

static int
step1(qlState **state, qlParameter *param)
{
	char x[1024];
	return step0(state, param, x);
}

int
main()
{
	/* NOTE: We alternate stepN() to test resuming/returning from
	 * different points in the stack. */
	int alternate, i, j;
	qlState *state = NULL;
	qlParameter param = NULL;
	const char * const *engines;
	qlFlags methods[] = {
		QL_FLAG_METHOD_COPY,
		QL_FLAG_METHOD_SHIFT,
		QL_FLAG_NONE
	};

	engines = ql_engine_list();
	assert(engines);

	for (i=0; engines[i]; i++) {
		qlFlags flags = ql_engine_get_flags(engines[i]);

		for (j=0; methods[j]; j++) {
			const char *method;

			if (!(flags & methods[j]))
				continue;
			if (methods[j] & QL_FLAG_METHOD_COPY)
				method = "copy";
			else if (methods[j] & QL_FLAG_METHOD_SHIFT)
				method = "shift";
			else {
				method = NULL; /* Make the compiler shut-up */
				assert(0);
			}

			alternate = 0;
			do {
				printf("\n%s/%s/%s\n", engines[i], method,
						               alternate % 2 == 0 ? "even" : "odd");
				param = (qlParameter) 0x1;
				assert((state = ql_state_new(engines[i], methods[j],
                                              level0, 0)));
				while (state) {
					if (alternate++ % 2 == 0)
						assert(step1(&state, &param) == 0);
					else
						assert(step0(&state, &param, NULL) == 0);

					if (state)
						printf("\tyielded : %p\n", param);
					else
						printf("\treturned: %p\n", param);
					DOUBLE(param);
				}
				assert(param == LASTVAL);
			} while (++alternate % 2 == 1);
		}
	}

	return 0;
}
