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

typedef struct qlState qlState;

/* A function which can be yield()ed from. */
typedef void *
(qlFunction)(qlState *state, void *misc);

/*
 * Calls or resumes the qlFunction.
 *
 * When you call ql_call() on a qlFunction/qlState for the first time (i.e.
 * when the pointer referenced by state is NULL) a qlState will be allocated
 * and passed to the qlFunction along with the pointer referenced by misc.
 *
 * If, during the execution of the qlFunction, ql_yield() is called, the
 * qlState* and the void* will be stored in the state and misc parameters of
 * ql_call() respectively and ql_call() will return 1. If you call ql_call()
 * subsequently on the same qlFunction/qlState, execution will resume at the
 * last ql_yield() with the pointer passed in the subsequent ql_call() stored
 * in the misc parameter of ql_yield().
 *
 * When the qlFunction returns, the returned pointer is stored in ql_call()'s
 * misc parameter, the state parameter will be set to NULL and ql_call() will
 * return 1. The qlState is automatically freed.
 *
 * If you wish to cancel execution (i.e. demand that the function return
 * immediately after cleanup), call ql_call() with misc == NULL. This causes
 * ql_yield() to return -1. It is the job of the function to cleanup and return
 * ASAP and it will have no choice but to do so since future ql_yield()s will
 * fail).
 *
 * ql_call() MUST be called on subsequent calls from the same EXACT position in
 * the stack as the first call. Any attempt to do otherwise will result in an
 * error condition (i.e. 0 will be returned), but the qlState will remain valid.
 * This also implies that nesting is generally speaking safe.
 *
 * Thus, the general pattern is something like this:
 *   qlState *state = NULL;
 *   void *misc = NULL;
 *   bool cancel = false;
 *   do {
 *     if (ql_call(myfunc, &state, cancel ? NULL : &misc)) {
 *       if (state) {
 *         // Function yielded
 *         if (iWantToCancel)
 *           cancel = true;
 *       } else
 *         // Function returned
 *     } else
 *     	// Error
 *   } while (state)
 *
 * ql_call() returns 0 (error) in the following conditions:
 *   1. malloc() fails (only on the first call).
 *   2. call is NULL.
 *   3. state is NULL.
 *   4. misc is NULL on the first call.
 *   5. If you attempt a subsequent ql_call() at a different stack location.
 *
 * @param call  The function to call.
 * @param state The pointer to store a state reference in.
 * @param misc  The memory to store a pointer to pass back and forth.
 * @return 1 on return or yield; 0 on failure
 */
int
ql_call(qlFunction *call, qlState **state, void** misc);

/*
 * Yields control back to ql_call().
 *
 * The pointer referenced by misc will be stored in ql_call()'s misc before it
 * returns. When ql_call() is called again, execution will resume at the last
 * ql_yield() and the pointer passed in ql_call()'s misc will be stored in
 * ql_yield()'s misc before it returns.
 *
 * If further calls are canceled, this function will return -1. If -1 is
 * returned, it is the job of the function to clean up and return immediately.
 * Any further attempts to call ql_yield() will fail.
 *
 * Thus, the general pattern is something like this:
 *   int ret = ql_yield(state, &misc);
 *   if (ret == 0)
 *     log(ENOMEM);
 *   if (ret < 1) {
 *     free(mem);
 *     close(fd);
 *     return NULL;
 *   }
 *   // Resume function here
 *
 * @param state The state reference to jump to.
 * @param misc  The memory to store a pointer to pass back and forth.
 * @return 1 on success; zero if malloc() fails; -1 if canceled
 */
int
ql_yield(qlState *state, void **misc);

#endif /* LIBQL_H_ */
