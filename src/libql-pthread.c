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
#include <limits.h>

#include <pthread.h>

typedef struct qlStatePThread {
  qlState state;
  pthread_barrier_t barrier;
  pthread_t thread;
  bool returned;
} qlStatePThread;

static void
eng_pthread_free(qlStatePThread *state)
{
  pthread_cancel(state->thread);
  pthread_join(state->thread, NULL);
  pthread_barrier_destroy(&state->barrier);
}

static void
barrier_wait(pthread_barrier_t *barrier)
{
  int status;
  status = pthread_barrier_wait(barrier);
  assert(status == PTHREAD_BARRIER_SERIAL_THREAD || status == 0);
}

static void *
inside_thread(qlStatePThread *state)
{
  qlParameter param = NULL;

  barrier_wait(&state->barrier);
  barrier_wait(&state->barrier);

  state->state.param = state->state.func(&state->state, state->state.param);
  state->returned = true;
  barrier_wait(&state->barrier);
  return param;
}

size_t
eng_pthread_size()
{
  return sizeof(qlStatePThread);
}

size_t
eng_pthread_align()
{
  return 0x10;
}

size_t
eng_pthread_stack()
{
  return PTHREAD_STACK_MIN / get_pagesize();
}

bool
eng_pthread_init(qlStatePThread *state)
{
  pthread_attr_t attr;

  state->returned = false;
  if (pthread_barrier_init(&state->barrier, NULL, 2) != 0)
    return false;

  if (pthread_attr_init(&attr) != 0) {
    pthread_barrier_destroy(&state->barrier);
    return false;
  }

  if (pthread_attr_setstack(&attr, state->state.stack,
                            sc_size(state->state.stack)) != 0) {
    pthread_barrier_destroy(&state->barrier);
    pthread_attr_destroy(&attr);
    return false;
  }

  if (pthread_create(&state->thread, &attr,
                     (void*(*)(void*)) inside_thread, state) != 0) {
    pthread_barrier_destroy(&state->barrier);
    pthread_attr_destroy(&attr);
    return false;
  }
  pthread_attr_destroy(&attr);

  barrier_wait(&state->barrier);
  sc_destructor_set(state, eng_pthread_free);
  return true;
}

bool
eng_pthread_step(qlStatePThread *state)
{
  barrier_wait(&state->barrier);
  barrier_wait(&state->barrier);
  return !state->returned;
}

void
eng_pthread_yield(qlStatePThread *state)
{
  barrier_wait(&state->barrier);
  barrier_wait(&state->barrier);
}
