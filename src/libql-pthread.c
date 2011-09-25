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
	int sockpair[2];
	pthread_t thread;
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

	state->sockpair[0] = -1;
	state->sockpair[1] = -1;
}

static void *
inside_thread(qlStatePThread **state)
{
	qlParameter param = NULL;
	char c = STATUS_CANCEL;

	param = (*state)->state.func((qlState**) state, *(*state)->state.param);
	assert(write((*state)->sockpair[1], &c, sizeof(c)) == sizeof(c));
	return param;
}

int
eng_pthread_step(qlStatePThread **state, qlParameter *param)
{
	char c = STATUS_OK;
	int status = STATUS_ERROR;
	size_t pagesize = get_pagesize();

	if ((*state)->sockpair[0] < 0) {
		pthread_attr_t attr;

		if (socketpair(AF_UNIX, SOCK_STREAM, 0, (*state)->sockpair) != 0)
			goto out_state;

		if (pthread_attr_init(&attr) != 0)
			goto out_sockets;

		if (pthread_attr_setstack(&attr,
				                  (void*) ((((uintptr_t) &(*state)[1])
							                / pagesize + 1)
							                * pagesize),
							      ((*state)->state.size
								    - sizeof(qlStatePThread))
								      / pagesize
                                      * pagesize) != 0) {
			pthread_attr_destroy(&attr);
			goto out_sockets;
		}

		if (pthread_create(&(*state)->thread, &attr,
				           (void*(*)(void*)) inside_thread, state) != 0) {
			pthread_attr_destroy(&attr);
			goto out_sockets;
		}
	} else {
		*(*state)->state.param = *param;
		(*state)->state.param  =  param;
		do {
			if (write((*state)->sockpair[0], &c, sizeof(c)) == sizeof(c))
				break;
			else
				c = STATUS_CANCEL;
		} while (errno == EAGAIN);
	}

	if (c == STATUS_OK) {
		do {
			errno = c = STATUS_OK;
			if (read((*state)->sockpair[0], &c, sizeof(c)) == sizeof(c))
				break;
			else
				c = STATUS_CANCEL;
		} while (errno == EAGAIN);
	}

	if (c == STATUS_CANCEL) {
		assert(pthread_join((*state)->thread, param) == 0);
		status = STATUS_OK;
		goto out_sockets;
	}

	return STATUS_OK;

out_sockets:
	close((*state)->sockpair[0]);
	close((*state)->sockpair[1]);
out_state:
	(*state)->state.free((*state)->state.ctx, *state, (*state)->state.size);
	*state = NULL;
	return status;
}

int
eng_pthread_yield(qlStatePThread **state, qlParameter *param)
{
	char c = STATUS_OK;
	qlParameter *ptmp;

	ptmp = (*state)->state.param;
	*(*state)->state.param = *param;
	(*state)->state.param  =  param;

	do {
		if (write((*state)->sockpair[1], &c, sizeof(c)) == sizeof(c))
			break;
		else
			c = STATUS_ERROR;
	} while (errno == EAGAIN);

	if (c == STATUS_ERROR) {
		(*state)->state.param = ptmp;
		return STATUS_ERROR;
	}

	do {
		errno = c = STATUS_OK;
		if (read((*state)->sockpair[1], &c, sizeof(c)) == sizeof(c))
			break;
		else
			c = STATUS_CANCEL;
	} while (errno == EAGAIN);

	return c == STATUS_OK ? STATUS_OK : STATUS_CANCEL;
}

void
eng_pthread_cancel(qlStatePThread **state)
{
	assert(pthread_cancel((*state)->thread) == 0);
	assert(pthread_join((*state)->thread, NULL) == 0);
	assert(close((*state)->sockpair[0]) == 0);
	assert(close((*state)->sockpair[1]) == 0);
}
