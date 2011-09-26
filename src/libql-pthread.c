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
#include <sys/types.h>
#include <sys/socket.h>
#include <limits.h> /* For PTHREAD_STACK_MIN */
#include <errno.h>
#include <unistd.h>

#include <pthread.h>

typedef struct qlStatePThread {
	qlState state;
	pthread_barrier_t barrier;
	pthread_t thread;
	int returned;
} qlStatePThread;

size_t
eng_pthread_size()
{
	size_t pagesize = get_pagesize();
	assert(sizeof(qlStatePThread) < pagesize);
	return PTHREAD_STACK_MIN + pagesize;
}

void
eng_pthread_init(qlStatePThread *state)
{
	size_t pagesize = get_pagesize();

	/* Assure that state was allocated sizeof(qlStatePThread) below a page */
	assert(((uintptr_t) state) / pagesize ==
          (((uintptr_t) state) + sizeof(qlStatePThread)) / pagesize);

	state->returned = 0;
	assert(pthread_barrier_init(&state->barrier, NULL, 2) == 0);
}

static void
barrier_wait(pthread_barrier_t *barrier)
{
	int status;
	status = pthread_barrier_wait(barrier);
	assert(status == PTHREAD_BARRIER_SERIAL_THREAD || status == 0);
}
#define barrier_wait(s) barrier_wait((&(*state)->barrier))

static void *
inside_thread(qlStatePThread **state)
{
	qlParameter param = NULL;

	param = (*state)->state.func((qlState**) state, *(*state)->state.param);
	(*state)->returned = 1;
	barrier_wait(state);
	return param;
}

int
eng_pthread_step(qlStatePThread **state, qlParameter *param)
{
	size_t pagesize = get_pagesize();

	if ((*state)->state.func) {
		pthread_attr_t attr;

		if (pthread_attr_init(&attr) != 0)
			return STATUS_ERROR;

		if (pthread_attr_setstack(&attr,
				                  (void*) ((((uintptr_t) &(*state)[1])
							                / pagesize + 1)
							                * pagesize),
							      ((*state)->state.size
								    - sizeof(qlStatePThread))
								      / pagesize
                                      * pagesize) != 0) {
			assert(pthread_attr_destroy(&attr) == 0);
			return STATUS_ERROR;
		}

		if (pthread_create(&(*state)->thread, &attr,
				           (void*(*)(void*)) inside_thread, state) != 0) {
			assert(pthread_attr_destroy(&attr) == 0);
			return STATUS_ERROR;
		}
		pthread_attr_destroy(&attr);
	} else {
		*(*state)->state.param = *param;
		(*state)->state.param  =  param;

		barrier_wait(state);
	}

	barrier_wait(state);

	if ((*state)->returned) {
		assert(pthread_join((*state)->thread, param) == 0);
		assert(pthread_barrier_destroy(&(*state)->barrier) == 0);
		(*state)->state.free((*state)->state.ctx, *state, (*state)->state.size);
		*state = NULL;
	}

	return STATUS_OK;
}

int
eng_pthread_yield(qlStatePThread **state, qlParameter *param)
{
	barrier_wait(state);
	barrier_wait(state);
	return (*state)->state.param ? STATUS_OK : STATUS_CANCEL;
}

void
eng_pthread_cancel(qlStatePThread **state)
{
	assert(pthread_cancel((*state)->thread) == 0);
	assert(pthread_join((*state)->thread, NULL) == 0);
	assert(pthread_barrier_destroy(&(*state)->barrier) == 0);
}
