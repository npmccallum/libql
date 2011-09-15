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

#ifndef LIBQLSP_H_
#define LIBQLSP_H_
#include <libql.h>

typedef struct qlStatePool qlStatePool;

/*
 * Creates a qlState pool of max size.
 *
 * After you have created a pool, you should initialize your qlStates using
 * ql_state_pool_state_init() to utilize memory from the pool.
 *
 * @param size The maximum number of qlState buffers to store in the pool.
 * @return A new qlStatePool or NULL on error.
 */
qlStatePool *
ql_state_pool_init(size_t size);

/*
 * Creates a qlState pool of max size with full control on memory life-cycle.
 *
 * After you have created a pool, you should initialize your qlStates using
 * ql_state_pool_state_init() to utilize memory from the pool.
 *
 * @param size The maximum number of qlState buffers to store in the pool.
 * @param resize Callback function to resize memory or NULL.
 * @param free Callback function to free memory or NULL.
 * @param ctx An opaque context to pass to resize and free.
 * @return A new qlStatePool or NULL on error.
 */
qlStatePool *
ql_state_pool_init_full(size_t size, qlResize *resize, qlFree *free, void *ctx);

/*
 * Initializes a qlState from the memory in the pool.
 *
 * If the pool is empty, new memory is allocated (using the resize callback).
 * If the pool is not full when the function executed by ql_state_step()
 * returns, the memory is placed back in the pool rather than freed. However,
 * if the pool is full, either the memory from the currently returned qlState
 * or one of the qlStates in the pool will be freed. The particular algorithm
 * for this behavior is undefined.
 *
 * @param pool The qlStatePool use memory from.
 * @param func The function to call in ql_state_step().
 * @param size The initial size of the stack buffer.
 * @return The new qlState or NULL on error.
 */
qlState *
ql_state_pool_state_init(qlStatePool *pool, qlMethod method,
                         qlFunction *func, size_t size);

/*
 * Frees a qlStatePool.
 *
 * If this function is called while there are outstanding qlStates which use
 * memory from this pool, the pool will actually be freed when all qlStates
 * are freed.
 *
 * @param pool The qlStatePool to free.
 */
void
ql_state_pool_free(qlStatePool *pool);

#endif /* LIBQLSP_H_ */
