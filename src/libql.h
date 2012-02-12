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

#define QL_FLAG_NONE            0

/*
 * Indicates to use the COPY method. Currently this method is only supported by
 * the assembly engine.
 *
 * This method copies the section of the stack we wish to resume in
 * ql_state_yield() into a buffer (which may be resized in ql_state_yield())
 * and will copy that buffer back onto the stack when we call ql_state_step()
 * to resume execution. The advantage of this method is that only the memory
 * that is needed will be allocated, which is very handy in low memory
 * situations.
 *
 * The first down-side is that the copy operation has roughly a 25%
 * performance penalty compared to the assembly engine's SHIFT method. In most
 * cases however this performance loss is negligible since:
 *   1. The assembly engine is by far the fastest engine (assembly's COPY
 *      method is still drastically faster than any other engine's SHIFT
 *      method).
 *   2. You are likely to be calling ql_state_yield() to avoid blocking IO.
 *
 * Second, and more crucial, is the fact that re-locating memory invalidates
 * all pointers to that memory. Because of this fact, ql_state_step() MUST be
 * called from roughly the same place in the stack or lower on each subsequent
 * call. We do pad this somewhat to avoid compiler issues and to provide some
 * flexibility. However, this buffer is not extremely large, so it is very
 * possible that ql_state_step() may error.
 *
 * Third, since allocation may occur in ql_state_yield(), it is possible that
 * the attempt to yield may fail and code must be written to handle this error
 * condition.
 *
 * When using the copy method, the size parameter determines the amount of copy
 * buffer to allocate. Some of the copy buffer will be used to store the qlState
 * data, so the minimum allocation is usually $PAGESIZE * 2, which varies on
 * different platforms.
 */
#define QL_FLAG_METHOD_COPY     ((qlFlags) (1 << 0))

/*
 * Indicates to use the SHIFT method. Currently this is supported by all
 * engines.
 *
 * In this method, a stack is pre-allocated (minimum is usually about 5-6
 * pages) and when ql_state_step() is called to begin/resume execution in the
 * qlFunction or its children we jump from the program default stack into our
 * pre-allocated one. This method has several benefits.
 *
 * First, SHIFT is about 25% faster than COPY in the assembly engine.
 *
 * Second, a qlState initialized as SHIFT can be resumed (ql_state_step()) from
 * anywhere, including a different process or thread (as long as it has
 * read/write permissions on the pre-allocated stack and the code segment
 * hasn't moved [not likely]).
 *
 * However, SHIFT also has some down-sides.
 *
 * First, the entire stack MUST be pre-allocated. This means that you must have
 * advance knowledge about how deep the stack will go or risk drastically
 * over-allocating your memory to compensate. This may seem trivial at first,
 * but if you allocate an 8M stack and have 20 qlStates running in parallel you
 * are requiring a lot of memory (160M) that you are likely never to use.
 * However, this over-allocation tends to be mitigated by on-demand allocation
 * in the kernel.
 *
 * Second, since the stack must be pre-allocated, if you don't allocate a big
 * enough stack you get a crash. When using the COPY if you run out of memory
 * you get a handy error return value in ql_state_yield().
 *
 * When using the shift method, the size parameter determines the size of the
 * initial stack. Some of this stack will be used for the qlState data.
 */
#define QL_FLAG_METHOD_SHIFT    ((qlFlags) (1 << 1))

/*
 * Indicates that any changes to the signal mask will be restored when jumping.
 */
#define QL_FLAG_RESTORE_SIGMASK ((qlFlags) (1 << 2))

/*
 * Indicates that the co-routine will execute in its own thread.
 */
#define QL_FLAG_THREADED        ((qlFlags) (1 << 3))

typedef void*          qlParameter;
typedef struct qlState qlState;
typedef uint32_t       qlFlags;

/* A function which can be yield()ed from. */
typedef qlParameter
qlFunction(qlState **state, qlParameter param);

/* A callback to resize the stack */
typedef void *
qlResize(void *ctx, void *mem, size_t size);

/* A callback to free the stack */
typedef void
qlFree(void *ctx, void *mem, size_t size);

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*
 * Iterate over the qlState implementations.
 *
 * This returns an array of strings, terminated by NULL, which list the names
 * of the engines built into this library. DO NOT attempt to free the array
 * or its contents.
 *
 * @see ql_engine_get_flags()
 * @see ql_state_new()
 * @see ql_state_new_full()
 * @return Array of engine names.
 */
const char * const *
ql_engine_list();

/*
 * Returns the supported flags for a given engine. Returns QL_FLAG_NONE if the
 * engine specified is not found.
 *
 * @see ql_engine_list()
 * @see ql_state_new()
 * @see ql_state_new_full()
 * @return Engine flags.
 */
qlFlags
ql_engine_get_flags(const char *eng);

/*
 * Initializes a coroutine to be called.
 *
 * You may safely nest qlState invocations. If eng is not NULL, than only the
 * specified engine will be used. If the engine is not provided in this build,
 * ql_state_new() will return NULL. If eng is NULL, an appropriate engine
 * will be chosen based upon the flags you specify. If you specify a set of
 * flags that are not provided by any of the engines, ql_state_new() will
 * return NULL.
 *
 * The size parameter specifies the amount of buffer to pre-allocate. Its
 * minimum size is engine specific. In general, this memory is allocated in
 * chunks of at least $PAGESIZE size.
 *
 * ENGINES
 *
 * Several different co-routine implementations are provided each with different
 * features. Not all engines are available in every build. You can find the
 * engines provided in this build by calling ql_engine_list().
 *
 * Possible engines include:
 *
 * assembly - FAST, provides both the COPY and SHIFT methods.
 * ucontext - SLOWER, broad architecture/platform support.
 * pthreads - SLOWEST, very broad architecture/platform support.
 *
 * Each engine may also provide different features. These features are
 * identifiable by flags. See the documentation for each flag for details.
 *
 *
 * @see ql_engine_list()
 * @see ql_engine_get_flags()
 * @see ql_state_new_full()
 * @see ql_state_step()
 * @see ql_state_yield()
 * @param eng The name of the engine desired.
 * @param flags The features desired.
 * @param func The function to call.
 * @param size The size of the stack or copy buffer to pre-allocate.
 * @return The qlState to step/yield.
 */
qlState *
ql_state_new(const char *eng, qlFlags flags, qlFunction *func, size_t size);

/*
 * Initializes a coroutine to be called with full control on memory life-cycle.
 *
 * This function is in all respects the same as ql_state_new() with the
 * exception that you can use it to gain fine-grained control over all
 * allocations, including providing an pre-allocated buffer.
 *
 * If the memory parameter is NULL, size behaves exactly like ql_state_new().
 * If the memory parameter is not NULL, size indicates the size of the buffer
 * pointed to by the memory parameter.
 *
 * @see ql_engine_list()
 * @see ql_engine_get_flags()
 * @see ql_state_new()
 * @see ql_state_step()
 * @see ql_state_yield()
 * @param eng The name of the engine desired.
 * @param flags The features desired.
 * @param func The function to call.
 * @param size The size of the stack/copy-buffer to allocate or the size of memory.
 * @param memory Pre-allocated memory of 'size' or NULL to pre-allocate.
 * @param resize Callback function to resize memory or NULL.
 * @param free Callback function to free memory or NULL.
 * @param ctx An opaque context to pass to resize and free.
 * @return The qlState to step/yield.
 */
qlState *
ql_state_new_full(const char *eng, qlFlags flags, qlFunction *func, size_t size,
                  void *memory, qlResize *resize, qlFree *free, void *ctx);

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
 * If QL_FLAG_METHOD_COPY is used, ql_state_step() MUST be called on subsequent
 * calls from near the same position in the stack as the first call. Any
 * attempt to do otherwise will result in an error condition (i.e. non-zero
 * will be returned), but the qlState will remain valid.
 *
 * Thus, the general pattern is something like this:
 *   qlState *state;
 *   qlParameter param;
 *
 *   state = ql_state_new(QL_METHOD_COPY, myFunc, 0);
 *   while (state) {
 *     if (ql_state_step(&state, &param)) {
 *       if (state) {
 *         // Function yielded
 *       } else
 *         // Function returned
 *     } else
 *     	// Error
 *   }
 *
 * @see ql_state_new()
 * @see ql_state_new_full()
 * @see ql_state_yield()
 * @param state The state object
 * @param param The parameter to pass back and forth
 * @return 0 on return or yield; non-zero on failure
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
 * Any further attempts to call ql_yield() will fail and return -1.
 *
 * Thus, the general pattern is something like this:
 *   int status = ql_state_yield(state, &param);
 *   if (status != 0) {
 *     if (status > 0)
 *       log("an error occurred");
 *     free(mem);
 *     close(fd);
 *     return param;
 *   }
 *   // Resume function here
 *
 * @see ql_state_new()
 * @see ql_state_new_full()
 * @see ql_state_step()
 * @see ql_state_cancel()
 * @param state The state reference to jump to.
 * @param misc  The memory to store a pointer to pass back and forth.
 * @return 0 on success; non-zero on error; -1 on cancellation
 */
int
ql_state_yield(qlState **state, qlParameter* param);

/*
 * Cancels execution of a co-routine.
 *
 * You do NOT need to call this after the function returns, only if you wish
 * to cancel the function during a ql_state_yield().
 *
 * If resume is non-zero, the co-routine will be resumed one last time and
 * ql_state_yield() will return -1. This indicates that the co-routine has been
 * cancelled and any required cleanup should be performed. It is the job of the
 * function to cleanup and return ASAP (and it will have no choice but to do
 * this since future ql_state_yield()s will fail).
 *
 * However, if resume is zero, the function will not be resumed and any
 * open resources will be leaked. This is generally helpful in the case of
 * using hierarchical memory allocators.
 *
 * @see ql_state_new()
 * @see ql_state_new_full()
 * @see ql_state_step()
 * @see ql_state_yield()
 */
void
ql_state_cancel(qlState **state, int resume);

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */
#endif /* LIBQL_H_ */
