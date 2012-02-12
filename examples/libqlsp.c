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

#include "libqlsp.h"

#include <stdlib.h>
#include <string.h>

typedef struct qlStatePoolItem qlStatePoolItem;
struct qlStatePoolItem {
	qlState *state;
	size_t    size;
};

struct qlStatePool {
	qlStatePoolItem *items;
	qlResize       *resize;
	qlFree	         *free;
	void              *ctx;
	size_t            size;
	size_t		      used;
	size_t			   ref;
	int           freeable;
};

static void *
int_resize(void *ctx, void *mem, size_t size)
{
	return realloc(mem, size);
}

static void
int_free(void *ctx, void *mem, size_t size)
{
	free(mem);
}

static void
pool_decref(qlStatePool *sp)
{
	if (!sp)
		return;

	sp->ref = sp->ref > 1 ? (sp->ref - 1) : 0;
	if (sp->ref > 0 || !sp->freeable)
		return;

	while (sp->used-- > 0)
		sp->free(sp->ctx, sp->items[sp->used].state, sp->items[sp->used].size);
	free(sp->items);
	free(sp);
}

static void *
pool_resize(qlStatePool *sp, void *state, size_t size)
{
	if (!sp)
		return NULL;

	return sp->resize(sp->ctx, state, size);
}

static void
pool_free(qlStatePool *sp, void *state, size_t size)
{
	if (sp->used < sp->size) {
		sp->items[sp->used].state = state;
		sp->items[sp->used++].size = size;
	} else
		sp->free(sp->ctx, state, size);
	pool_decref(sp);
}

qlStatePool *
ql_state_pool_new(size_t size)
{
	return ql_state_pool_new_full(size, NULL, NULL, NULL);
}

qlStatePool *
ql_state_pool_new_full(size_t size, qlResize *resize, qlFree *freef, void *ctx)
{
	qlStatePool *sp = NULL;
	if (!resize)
		resize = int_resize;
	if (!freef)
		freef = int_free;

	sp = (*resize)(ctx, NULL, sizeof(qlStatePool));
	if (!sp)
		return NULL;
	memset(sp, 0, sizeof(qlStatePool));
	sp->items = (*resize)(ctx, NULL, size * sizeof(qlStatePoolItem));
	if (!sp->items) {
		free(sp);
		return NULL;
	}

	sp->resize = resize;
	sp->free   = freef;
	sp->ctx    = ctx;
	sp->size   = size;
	return sp;
}

qlState *
ql_state_pool_state_new(qlStatePool *sp, const char *eng, qlFlags flags,
                         qlFunction *func, size_t size)
{
	size_t i, smallest;
	qlState *state = NULL, *tmp = NULL;

	/* If we have a buffer available, try to reuse it */
	if (sp->used > 0) {
		/* First try to find one that is at least our required size. */
		for (smallest=i=0; i < sp->size; i++) {
			if (sp->items[i].size >= size) {
				state = sp->items[i].state;
				size  = sp->items[i].size;
				sp->items[i] = sp->items[--(sp->used)];
				break;
			} else if (sp->items[i].size > sp->items[smallest].size)
				smallest = i;
		}

		if (!state) {
			/* Resize the smallest one to increase our cache hit rate */
			state = sp->resize(sp->ctx, sp->items[smallest].state, size);
			if (state) /* If the resize worked, remove the item */
				sp->items[smallest] = sp->items[--(sp->used)];
		}
	}

	tmp =  ql_state_new_full(eng, flags, func, size, state,
			                  (qlResize*) pool_resize,
			                  (qlFree*) pool_free, sp);
	if (tmp)
		sp->ref++;
	else if (state) {
		sp->items[++(sp->used)].state = state;
		sp->items[sp->used].size = size;
	}
	return tmp;
}

void
ql_state_pool_free(qlStatePool *sp)
{
	if (!sp)
		return;
	sp->freeable = 1;
	if (sp->ref == 0)
		pool_decref(sp);
}
