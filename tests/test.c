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

#include <libsc.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define DOUBLE(v) v = (qlParameter) (((uintptr_t) v) * 2);
#define LASTVAL   ((qlParameter) 0x1000)

static qlParameter
level2(qlState *state, qlParameter *param)
{
  DOUBLE(*param);
  printf("\tlevel2-1: %p\n", *param);

  DOUBLE(*param);
  ql_state_yield(state, param);
  printf("\tlevel2-2: %p\n", *param);

  DOUBLE(*param);
  return *param;
}

static qlParameter
level1(qlState *state, qlParameter *param)
{
  DOUBLE(*param);
  printf("\tlevel1-1: %p\n", *param);

  DOUBLE(*param);
  ql_state_yield(state, param);
  printf("\tlevel1-2: %p\n", *param);

  *param = level2(state, param);
  printf("\tlevel1-3: %p\n", *param);

  DOUBLE(*param);
  return *param;
}

static qlParameter
level0(qlState *state, qlParameter param)
{
  DOUBLE(param);
  printf("\tlevel0-1: %p\n", param);
  DOUBLE(param);
  ql_state_yield(state, &param);
  printf("\tlevel0-2: %p\n", param);

  param = level1(state, &param);
  printf("\tlevel0-3: %p\n", param);
  DOUBLE(param);
  return param;
}

int
main()
{
  /* NOTE: We alternate stepN() to test resuming/returning from
   * different points in the stack. */
  const char * const *engines;

  engines = ql_engine_list();
  assert(engines);

  for (int i = 0; engines[i]; i++) {
    qlParameter param = (qlParameter) 0x1;
    qlState *state;

    printf("\n%s\n", engines[i]);
    state = ql_state_new(NULL, engines[i], level0, 0);
    assert(state);

    while (ql_state_step(state, &param)) {
      printf("\tyielded : %p\n", param);
      DOUBLE(param);
    }

    printf("\treturned: %p\n", param);
    sc_decref(NULL, state);
    assert(param == LASTVAL);
  }

  return 0;
}
