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

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#ifdef _WIN32
#define CCONV __cdecl
#else
#define CCONV
#endif

typedef struct {
  qlState state;
  jmp_buf  step;
  jmp_buf yield;
} qlStateSetJmp;

qlStateSetJmp *CCONV
call_function(qlStateSetJmp *state, qlParameter *param,
              qlFunction *func, void *stack, size_t size);

size_t
eng_setjmp_size()
{
  return sizeof(qlStateSetJmp);
}

bool
eng_setjmp_init(qlStateSetJmp *state)
{
  return true;
}

bool
eng_setjmp_step(qlStateSetJmp *state)
{
  int result = 0;

  /* Store the current state */
  result = setjmp(state->step);
  if (result != 0)
    return result == STATUS_YIELD;

  if (!state->state.func) {
    longjmp(state->yield, 1);
    return false; /* Make the compiler happy; should never happen */
  }

  /* We can't trust the stack after call_function,
   * so jump from the return value copy of state. */
  longjmp(call_function(state, &state->state.param, state->state.func,
                        state->state.stack, sc_size(state->state.stack))->step,
          STATUS_RETURN);
  return false; /* Make the compiler happy; should never happen */
}

void
eng_setjmp_yield(qlStateSetJmp *state)
{
  /* Store our state */
  if (setjmp(state->yield) == 0)
    longjmp(state->step, STATUS_YIELD); /* Never returns */
}
