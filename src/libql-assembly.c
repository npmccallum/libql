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

#define DIFF(src, dst)  (labs(dst - src))
#define START(one, two) ((one) < (two) ? (one) : (two))
#define ALIGN(ptr, pagesize, next) \
	((ptr / pagesize + (next ? 1 : 0)) * pagesize)

typedef struct qlStateAssembly {
	qlState     state;
	int        stkdir;
	size_t   pagesize;
	jmp_buf    stpbuf;
	jmp_buf    yldbuf;
	uintptr_t  stppos;
	uintptr_t  yldpos;
} qlStateAssembly;

int CCONV
get_stack_direction(void);

uintptr_t CCONV
get_stack(void);

qlParameter * CCONV
translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);

void CCONV
resume_function(jmp_buf yldbuf, jmp_buf stpbuf, uintptr_t dst,
                uintptr_t src, void *buf, qlParameter *param);

void CCONV
call_function(qlStateAssembly **state, qlParameter *param, qlFunction *func,
              uintptr_t stack, jmp_buf stpbuf);

/* This little helper function exists because some platforms put
 * longjmp in the PLT and I'd rather not write code to detect it. */
void
dolongjmp(jmp_buf state, int value)
{
	longjmp(state, value);
}

size_t
eng_assembly_size()
{
	assert(sizeof(qlStateAssembly) < get_pagesize());
	return get_pagesize();
}

void
eng_assembly_init(qlStateAssembly *state)
{
	((qlStateAssembly*) state)->stkdir = get_stack_direction();
	((qlStateAssembly*) state)->pagesize = get_pagesize();
	((qlStateAssembly*) state)->yldpos = 0; /* For NULL detection later */

	/* Assure that state was allocated sizeof(qlStateCopy) below a page */
	assert(((uintptr_t) state) / ((qlStateAssembly*) state)->pagesize ==
          (((uintptr_t) state) + sizeof(qlStateAssembly)) /
          	  ((qlStateAssembly*) state)->pagesize);
}

int
eng_assembly_step(qlStateAssembly **state, qlParameter *param)
{
	/* Store the current state */
	switch (setjmp((*state)->stpbuf)) {
	case 0: /* We have marked our state. */
		break;
	case 1: /* The function has returned. */
		(*state)->state.free((*state)->state.ctx, *state, (*state)->state.size);
		*state = NULL;
		return STATUS_OK;
	case 2: /* We are resuming from ql_state_yield(). */
		return STATUS_OK;
	default:
		assert(0);
		break;
	}

	if (!(*state)->state.func) {
		/* Ensure we are restoring from lower in the stack */
		if ((*state)->state.flags & QL_FLAG_METHOD_COPY) {
			if (get_stack_direction() > 0
					? get_stack() > (*state)->stppos
					: get_stack() < (*state)->stppos)
				return STATUS_ERROR;
		}

		/* Set the parameter */
		if (param) {
			qlParameter *tmp;
			tmp = translate((*state)->yldpos, (*state)->stppos,
					        &(*state)[1], (*state)->state.param);
			*tmp = *param;
		}
		(*state)->state.param = param;

		resume_function((*state)->yldbuf, (*state)->stpbuf,
                        (*state)->yldpos, (*state)->stppos,
                        ((*state)->state.flags & QL_FLAG_METHOD_SHIFT)
                        	? NULL : &(*state)[1], param);
		assert(0); /* We will never get here */
	}

	/* Figure out where our execution stack will lie */
	if ((*state)->state.flags & QL_FLAG_METHOD_SHIFT)
		(*state)->stppos = get_stack_direction() > 0
							? ((uintptr_t) *state)
							: ((uintptr_t) *state) + (*state)->state.size;
	else
		(*state)->stppos = get_stack_direction() > 0
							? get_stack() + (*state)->pagesize
							: get_stack() - (*state)->pagesize;
	(*state)->stppos = ALIGN((*state)->stppos, (*state)->pagesize,
                              get_stack_direction() > 0);

	if (get_stack_direction() > 0)
		/* Next page */
		(*state)->stppos  = ((*state)->stppos / get_pagesize() + 1)
												* get_pagesize();
	else {
		/* Last page */
		(*state)->stppos += (*state)->state.size;
		(*state)->stppos  = (*state)->stppos / get_pagesize()
											   * get_pagesize();
	}

	call_function(state, param, (*state)->state.func,
                  (*state)->stppos, (*state)->stpbuf);
	assert(0); /* We will never get here */
	return STATUS_ERROR; /* Make the compiler happy */
}

int
eng_assembly_yield(qlStateAssembly **state, qlParameter *param)
{
	size_t needed;
	int retval;

	/* Store our state */
	retval = setjmp((*state)->yldbuf);
	if (retval == STATUS_RESUME)
		return STATUS_OK;
	if (retval == STATUS_CANCEL)
		return STATUS_CANCEL;

	if ((*state)->state.flags & QL_FLAG_METHOD_COPY) {
		/* Mark the high watermark of the stack to save */
		(*state)->yldpos = get_stack();

		/* Reallocate the buffer if need be */
		needed = sizeof(qlState) + DIFF((*state)->stppos, (*state)->yldpos);
		if ((*state)->state.size < needed) {
			qlStateAssembly *tmp;

			if (!(*state)->state.resize)
				return STATUS_ERROR;

			tmp = (*state)->state.resize((*state)->state.ctx, *state, needed);
			if (!tmp)
				return STATUS_ERROR;

			*state = tmp;
			(*state)->state.size = needed;
		}

		/* Copy stack into the heap */
		memcpy(&(*state)[1], (void*) START((*state)->stppos, (*state)->yldpos),
		       DIFF((*state)->stppos, (*state)->yldpos));
	}

	longjmp((*state)->stpbuf, 2);
	assert(0); /* We will never get here */
	return STATUS_ERROR; /* Make the compiler happy */
}

void
eng_assembly_cancel(qlStateAssembly **state)
{

}
