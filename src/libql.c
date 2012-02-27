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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define MAXENGINES 32
#define ENGINE_DEFINITIONS(name) \
  size_t eng_ ## name ## _size(void); \
  bool   eng_ ## name ## _init(qlState *); \
  bool   eng_ ## name ## _step(qlState *); \
  void   eng_ ## name ## _yield(qlState *);
#define ENGINE_ENTRY(name) { \
  # name, eng_ ## name ## _size, \
  eng_ ## name ## _init, \
  eng_ ## name ## _step, \
  eng_ ## name ## _yield \
}

struct qlStateEngine {
  const char *name;
  size_t (*size)(void);
  bool   (*init)(qlState *);
  bool   (*step)(qlState *);
  void   (*yield)(qlState *);
};

#ifdef WITH_ASSEMBLY
ENGINE_DEFINITIONS(assembly);
#endif
#ifdef WITH_UCONTEXT
ENGINE_DEFINITIONS(ucontext);
#endif
#ifdef WITH_PTHREAD
ENGINE_DEFINITIONS(pthread);
#endif

static const qlStateEngine engines[] = {
#ifdef WITH_ASSEMBLY
  ENGINE_ENTRY(assembly),
#endif
#ifdef WITH_UCONTEXT
  ENGINE_ENTRY(ucontext),
#endif
#ifdef WITH_PTHREAD
  ENGINE_ENTRY(pthread),
#endif
  { NULL, NULL, NULL, NULL, NULL }
};

static size_t
get_pagesize()
{
  static size_t pagesize = 0;

  if (pagesize == 0) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    pagesize = si.dwAllocationGranularity;
#else
    pagesize = sysconf(_SC_PAGESIZE);
#endif /* _WIN32 */
  }

  return pagesize;
}

const char * const *
ql_engine_list()
{
  static const char *enames[MAXENGINES + 1] = { NULL };

  if (enames[0] == NULL && engines[0].name != NULL) {
    int i;
    for (i = 0; i < MAXENGINES && engines[i].name; i++)
      enames[i] = engines[i].name;
    enames[i] = NULL;
  }

  return enames;
}

qlState *
ql_state_new(void *parent, const char *eng, qlFunction *func, size_t size)
{
  const qlStateEngine *engine = NULL;
  qlState *state;
  int i;

  if (!func)
    return NULL;

  if (size < 4 * get_pagesize())
    size = 16 * get_pagesize();

  for (i = 0; engines[i].name; i++) {
    if (!eng || !strcmp(engines[i].name, eng)) {
      engine = &engines[i];
      break;
    }
  }
  if (!engine)
    return NULL;

  state = sc_malloc0(parent, engine->size(), "qlState");
  if (!state)
    return NULL;

  state->eng = engine;
  state->func = func;
  state->stack = sc_memalign(state, get_pagesize(), size, "qlStack");
  if (!state->stack) {
    sc_decref(parent, state);
    return NULL;
  }

  if (!state->eng->init(state)) {
    sc_decref(parent, state);
    return NULL;
  }

  return state;
}

bool
ql_state_step(qlState *state, qlParameter* param)
{
  bool rslt;

  assert(state);

  state->param = param ? *param : NULL;
  rslt = state->eng->step(state);

  if (param)
    *param = state->param;
  return rslt;
}

void
ql_state_yield(qlState *state, qlParameter* param)
{
  assert(state);

  state->func = NULL;
  state->param = param ? *param : NULL;
  state->eng->yield(state);

  if (param)
    *param = state->param;
}
