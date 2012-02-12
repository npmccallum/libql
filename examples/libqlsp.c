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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct qlStatePoolItem qlStatePoolItem;
struct qlStatePoolItem {
  qlState *state;
  size_t    size;
  bool      used;
};

struct qlStatePool {
  qlStatePoolItem *items;
  qlResize       *resize;
  void              *ctx;
  size_t            size;
  size_t            used;
  size_t             ref;
  int           freeable;
};

static void *
int_resize(void *ctx, void *mem, size_t size)
{
  if (size == 0) {
    free(mem);
    return NULL;
  }

  return realloc(mem, size);
}

static void
pool_decref(qlStatePool *sp)
{
  int i;

  if (!sp)
    return;

  sp->ref = sp->ref > 1 ? (sp->ref - 1) : 0;
  if (sp->ref > 0 || !sp->freeable)
    return;

  for (i = 0; i < sp->size; i++)
    if (sp->items[i].state)
      sp->resize(sp->ctx, sp->items[i].state, 0);
  free(sp->items);
  free(sp);
}

static void *
pool_resize(qlStatePool *sp, void *state, size_t size)
{
  void *tmp;
  int i;
  qlStatePoolItem *item = NULL;

  if (!sp)
    return NULL;

  for (i = 0; i < sp->size; i++) {
    if (state && sp->items[i].state == state) {
      item = &sp->items[i];
      break;
    }
  }

  if (size == 0) {
    if (item) {
      item->used = false;
      sp->used--;
    } else
      sp->resize(sp->ctx, state, 0);

    pool_decref(sp);
    return NULL;
  }

  tmp = sp->resize(sp->ctx, state, size);
  if (tmp && item) {
    item->state = tmp;
    item->size  = size;
  }

  return tmp;
}

qlStatePool *
ql_state_pool_new(size_t size)
{
  return ql_state_pool_new_full(size, NULL, NULL);
}

qlStatePool *
ql_state_pool_new_full(size_t size, qlResize *resize, void *ctx)
{
  qlStatePool *sp = NULL;
  if (!resize)
    resize = int_resize;

  sp = (*resize)(ctx, NULL, sizeof(qlStatePool));
  if (!sp)
    return NULL;
  memset(sp, 0, sizeof(qlStatePool));
  sp->items = (*resize)(ctx, NULL, size * sizeof(qlStatePoolItem));
  if (!sp->items) {
    free(sp);
    return NULL;
  }
  memset(sp->items, 0, size * sizeof(qlStatePoolItem));

  sp->resize = resize;
  sp->ctx = ctx;
  sp->size = size;
  return sp;
}

qlState *
ql_state_pool_state_new(qlStatePool *sp, const char *eng, qlFlags flags,
                        qlFunction *func, size_t size)
{
  ssize_t i;
  qlState *state = NULL, *tmp = NULL;
  qlStatePoolItem *smallest = NULL;

  /* If we have a buffer available, try to reuse it */
  if (sp->size - sp->used > 0) {
    /* First try to find one that is at least our required size. */
    for (i = 0; i < sp->size; i++) {
      if (sp->items[i].used)
        continue;

      if (sp->items[i].size > 0 && sp->items[i].size >= size) {
        state = sp->items[i].state;
        size = sp->items[i].size;
        sp->items[i].used = true;
        sp->used++;
        break;
      } else if (!smallest || sp->items[i].size > smallest->size)
        smallest = &sp->items[i];
    }

    if (!state && smallest >= 0) {
      /* Resize the smallest one to increase our cache hit rate */
      state = sp->resize(sp->ctx, smallest->state, size);
      if (state) { /* If the resize worked, remove the item */
        smallest->used = true;
        smallest->size = size;
        smallest->state = state;
        sp->used++;
      }
    }
  }

  tmp = ql_state_new_full(eng, flags, func, size, state,
                          (qlResize*) pool_resize, sp);
  if (tmp)
    sp->ref++;
  else if (state) {
    for (i = 0; i < sp->size; i++) {
      if (sp->items[i].state == state) {
        sp->items[i].used = false;
        break;
      }
    }
    sp->used--;
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
