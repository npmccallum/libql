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

#include <libsc.h>

#include <stdbool.h>
#include <stddef.h>

typedef void *qlParameter;
typedef struct qlState qlState;

/* A function which can be yield()ed from. */
typedef qlParameter
qlFunction(qlState *state, qlParameter param);

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Iterate over the qlState implementation engines.
 *
 * This returns an array of strings, terminated by NULL, which list the names
 * of the engines built into this library. DO NOT attempt to free the array
 * or its contents.
 *
 * Several different co-routine implementations are provided with varying
 * performance. Not all engines are available in every build.
 *
 * Possible engines include:
 *   setjmp   - FAST - limited CPU architecture support (due to assembly code).
 *   ucontext - FAST - broad architecture/platform support.
 *   pthread  - SLOW - very broad architecture/platform support.
 *
 * @see ql_engine_get_flags()
 * @see ql_state_new()
 * @return Array of engine names.
 */
const char * const *
ql_engine_list();

/*
 * Initializes a coroutine to be called.
 *
 * You may safely nest qlState invocations.
 *
 * If eng is NULL, the fastest engine available in the current build will be
 * used. Otherwise, eng should be the name of an engine in the set returned by
 * ql_engine_list(). If an invalid engine is specified, this function returns
 * NULL.
 *
 * The size parameter specifies the amount of buffer to pre-allocate, which
 * must be a multiple of $PAGESIZE and at least 4 pages. If size is less than
 * the minimum size (4 pages), than the minimum size will be used. Use caution
 * in choosing your stack size to prevent crashes and data corruption.
 *
 * The qlState must be freed using the standard libsc conventions. Because of
 * this, it is wise to allocate resources using standard libsc conventions as
 * children of the qlState. This ensures that if the co-routine is cancelled,
 * proper cleanup will happen for all resources allocated within the co-routine.
 *
 * @see ql_engine_list()
 * @see ql_state_step()
 * @param parent The memory parent (libsc)
 * @param eng The name of the engine desired or NULL.
 * @param func The function to call.
 * @param size The size of the stack to pre-allocate.
 * @return The qlState to step/yield.
 */
qlState *
ql_state_new(void *parent, const char *eng, qlFunction *func, size_t size);

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
 * ql_state_step() stored in the param parameter of ql_state_yield(). When the
 * qlFunction returns the returned qlParameter is stored in ql_state_step()'s
 * param parameter.
 *
 * Thus, the general pattern is something like this:
 *   qlState *state;
 *   qlParameter param = 0;
 *
 *   state = ql_state_new(NULL, NULL, myFunc, 16 * PAGESIZE);
 *   while (ql_state_step(state, &param)) {
 *     // Function yielded, do something
 *   }
 *   // Function returned, do something
 *
 *   sc_decref(NULL, state); // Free the state
 *
 * @see ql_state_new()
 * @see ql_state_yield()
 * @param state The state object
 * @param param The parameter to pass back and forth
 * @return true if the function may be resumed, false if it has returned
 */
bool
ql_state_step(qlState *state, qlParameter *param);

/*
 * Yields control back to ql_state_step().
 *
 * The parameter will be stored in ql_state_step()'s param before it returns.
 * When ql_state_step() is called again, execution will resume at the last
 * ql_state_yield() and the parameter passed by ql_state_step() will be stored
 * in ql_state_yield()'s param before it returns.
 *
 * Thus, the general pattern is something like this:
 *   qlParameter
 *   myFunc(qlState *state, qlParameter param)
 *   {
 *     // Do something...
 *     ql_state_yield(state, &param);
 *     // Do something else...
 *     ql_state_yield(state, &param);
 *     // Do something else, again...
 *     return NULL;
 *   }
 *
 * @see ql_state_new()
 * @see ql_state_step()
 * @param state The state object
 * @param param The parameter to pass back and forth.
 */
void
ql_state_yield(qlState *state, qlParameter *param);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* LIBQL_H_ */
