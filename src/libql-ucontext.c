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

typedef struct qlStateUContext {
  qlState      state;
  volatile int jumped;
  ucontext_t   stpctx;
  ucontext_t   yldctx;
} qlStateUContext;

/* This should work even on a 128bit system. */
typedef union {
  struct {
    int a, b, c, d;
  } num  __attribute__ ((packed));
  qlStateUContext *state;
} pointerPasser;

size_t
eng_ucontext_size()
{
  return sizeof(qlStateUContext);
}

size_t
eng_ucontext_align()
{
  return 0x10;
}

size_t
eng_ucontext_stack()
{
  return 0x04;
}

bool
eng_ucontext_init(qlStateUContext *state)
{
  return sizeof(void*) <= sizeof(pointerPasser);
}

static void
inside_context(int a, int b, int c, int d)
{
  pointerPasser pp = { .num = { a, b, c, d } };
  pp.state->state.param = pp.state->state.func(&pp.state->state,
                                               pp.state->state.param);
  pp.state->jumped = STATUS_RETURN;
}

bool
eng_ucontext_step(qlStateUContext *state)
{
  state->jumped = 0;
  assert(getcontext(&state->stpctx) == 0);

  if (state->jumped != 0)
    return state->jumped == STATUS_YIELD;

  if (state->state.func) {
    pointerPasser pp;
    pp.state = state;

    assert(getcontext(&state->yldctx) == 0);
    state->yldctx.uc_link = &state->stpctx;
    state->yldctx.uc_stack.ss_size = sc_size(state->state.stack);
    state->yldctx.uc_stack.ss_sp = state->state.stack;

    /* Encode the pointer into the args buffer and execute. */
    makecontext(&state->yldctx, (void (*)(void)) inside_context,
                4, pp.num.a, pp.num.b, pp.num.c, pp.num.d);
  } else
    state->jumped = -1; /* Resume */

  assert(setcontext(&state->yldctx) == 0);
  return false; /* Never get here */
}

void
eng_ucontext_yield(qlStateUContext *state)
{
  state->jumped = 0;
  assert(getcontext(&state->yldctx) == 0);

  if (state->jumped == 0) {
    state->jumped = STATUS_YIELD;
    assert(setcontext(&state->stpctx) == 0);
  }
}

void
eng_ucontext_cancel(qlStateUContext **state)
{

}
