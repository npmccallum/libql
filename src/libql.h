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

#ifndef LIBQL_H_
#define LIBQL_H_

#include <stdint.h>
#include <stddef.h>

typedef struct qlState qlState;

typedef union qlParameter {
	void    *pointer;
	uint64_t uint64;
	int64_t   int64;
	uint32_t uint32;
	int32_t   int32;
	uint16_t uint16;
	int16_t   int16;
	uint8_t  uint8;
	int8_t    int8;
	double   dbl;
	float    flt;
} qlParameter;

/* A function which can be yield()ed from. */
typedef qlParameter
(qlFunction)(qlState **state, qlParameter param);

/* A callback to resize the stack */
typedef void *
(qlResize)(void *ctx, void *mem, size_t size);

/* A callback to free the stack */
typedef void
(qlFree)(void *ctx, void *mem, size_t size);

/*
 * Initializes a coroutine to be called.
 *
 * @see ql_state_step()
 * @see ql_state_yield()
 * @param func The function to call.
 * @return The qlState to call/yield.
 */
qlState *
ql_state_init(qlFunction *func);

/*
 * Initializes a coroutine to be called with a pre-allocated stack.
 *
 * @see ql_state_step()
 * @see ql_state_yield()
 * @param func The function to call.
 * @param size The size of the stack to pre-allocate.
 * @return The qlState to step/yield.
 */
qlState *
ql_state_init_size(qlFunction *func, size_t size);

/*
 * Initializes a coroutine to be called with full control on memory life-cycle.
 *
 * @see ql_state_step()
 * @see ql_state_yield()
 * @param func The function to call.
 * @param size The size of the stack to pre-allocate or the size of memory.
 * @param memory Pre-allocated memory of 'size' or NULL to pre-allocate.
 * @param resize Callback function to resize memory or NULL.
 * @param free Callback function to free memory or NULL.
 * @param ctx An opaque context to pass to resize and free.
 * @return The qlState to step/yield.
 */
qlState *
ql_state_init_full(qlFunction *func, size_t size, void *memory,
		           qlResize *resize, qlFree *free, void *ctx);

/*
 * Steps through the qlFunction.
 *
 * When you call ql_state_step() on a qlState for the first time the qlFunction
 * will be called and passed the qlState and the qlParameter referenced by the
 * qlParameter *param.
 *
 * If, during the execution of the qlFunction, ql_state_yield() is called, the
 * qlParameter will be stored in the param parameter and ql_state_step() will
 * return 1. When you call ql_state_step() again, execution will resume at the
 * last ql_state_yield() with the qlParameter passed in the subsequent
 * ql_state_step() stored in the param parameter of ql_state_yield().
 *
 * When the qlFunction returns:
 *  1. the returned qlParameter is stored in ql_state_step()'s param parameter
 *  2. the qlState is freed (via the callback)
 *  3. the state parameter will be set to NULL
 *
 * If you wish to cancel execution (i.e. demand that the function return
 * immediately after cleanup), call ql_state_step() with param == NULL. This
 * causes ql_state_yield() to return -1. It is the job of the function to
 * cleanup and return ASAP (and it will have no choice but to do this since
 * future ql_state_yield()s will fail).
 *
 * ql_state_step() MUST be called on subsequent calls from the same EXACT
 * position in the stack as the first call. Any attempt to do otherwise will
 * result in an error condition (i.e. 0 will be returned), but the qlState
 * will remain valid. This also implies that nesting is generally speaking safe.
 *
 * Thus, the general pattern is something like this:
 *   qlState *state;
 *   qlParameter param;
 *   bool cancel = false;
 *
 *   state = ql_state_init(myFunc);
 *   while (state) {
 *     if (ql_state_step(&state, cancel ? NULL : &param)) {
 *       if (state) {
 *         // Function yielded
 *         if (iWantToCancel)
 *           cancel = true;
 *       } else
 *         // Function returned
 *     } else
 *     	// Error
 *   }
 *
 * ql_state_step() returns 0 (error) in the following conditions:
 *   1. state or *state is NULL.
 *   2. param is NULL on the first call.
 *   3. subsequent ql_state_step() call from a different stack location.
 *
 * @param state The state object
 * @param param The parameter to pass back and forth
 * @return 1 on return or yield; 0 on failure
 */
int
ql_state_step(qlState **state, qlParameter* param);

/*
 * Yields control back to ql_state_step().
 *
 * The parameter will be stored in ql_state_step()'s param before it returns.
 * When ql_state_step() is called again, execution will resume at the last
 * ql_state_yield() and the parameter passed by ql_state_step() will be stored
 * in ql_state_yield()'s param before it returns.
 *
 * If further calls are canceled, this function will return -1. If -1 is
 * returned, it is the job of the function to clean up and return immediately.
 * Any further attempts to call ql_yield() will fail.
 *
 * Thus, the general pattern is something like this:
 *   int ret = ql_state_yield(state, &param);
 *   if (ret == 0)
 *     log(ENOMEM);
 *   if (ret < 1) {
 *     free(mem);
 *     close(fd);
 *     return param;
 *   }
 *   // Resume function here
 *
 * This function can fail (0 return value) due to the following:
 *   1. More memory on the stack is needed, but resize isn't defined.
 *   2. More memory on the stack is needed, but resize failed.
 *   3. A previous ql_state_yield() returned -1.
 *
 * @param state The state reference to jump to.
 * @param misc  The memory to store a pointer to pass back and forth.
 * @return 1 on success; 0 on error; -1 if canceled
 */
int
ql_state_yield(qlState **state, qlParameter* param);

#endif /* LIBQL_H_ */
