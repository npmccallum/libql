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

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

static qlParameter
level1(qlState **state, qlParameter *param)
{
	param->uint32 *= 2;
	printf("level1-1: %d\n", param->uint32);
	param->uint32 *= 2;
	ql_state_yield(state, param);
	printf("level1-2: %d\n", param->uint32);
	param->uint32 *= 2;
	return *param;
}

static qlParameter
level0(qlState **state, qlParameter param)
{
	param.uint32 *= 2;
	printf("level0-1: %d\n", param.uint32);
	param.uint32 *= 2;
	assert(ql_state_yield(state, &param) > 0);
	printf("level0-2: %d\n", param.uint32);

	param = level1(state, &param);
	printf("level0-3: %d\n", param.uint32);
	param.uint32 *= 2;
	return param;
}

int
main()
{
	qlState *state = NULL;
	qlParameter param;
	param.uint32 = 0x1;

	assert((state = ql_state_init(level0)));
	while (state) {
		assert(ql_state_step(&state, &param));
		if (state)
			printf("yielded : %d\n", param.uint32);
		else
			printf("returned: %d\n", param.uint32);
		param.uint32 *= 2;
	}

	return 0;
}
