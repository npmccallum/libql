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
	size_t name ## _size(void); \
	void   name ## _init(qlState *); \
	int    name ## _step(qlState **, qlParameter *); \
	int    name ## _yield(qlState **, qlParameter *)
#define ENGINE_ENTRY(flags, name) \
	{ flags, # name, name ## _size, \
      name ## _init, name ## _step, name ## _yield }

struct qlStateEngine {
	qlFlags     flags;
	const char *name;
	size_t    (*size)(void);
	void      (*init)(qlState *);
	int       (*step)(qlState **, qlParameter *);
	int       (*yield)(qlState **, qlParameter *);
};

#ifdef WITH_ASSEMBLY
ENGINE_DEFINITIONS(assembly);
#endif
#ifdef WITH_UCONTEXT
ENGINE_DEFINITIONS(ucontext);
#endif

static const qlStateEngine engines[] = {
#ifdef WITH_ASSEMBLY
	ENGINE_ENTRY(QL_FLAG_METHOD_COPY | QL_FLAG_METHOD_SHIFT, assembly),
#endif
#ifdef WITH_UCONTEXT
	ENGINE_ENTRY(QL_FLAG_METHOD_SHIFT | QL_FLAG_RESTORE_SIGMASK, ucontext),
#endif
	{0, 0, 0, 0, 0, 0}
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

size_t
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
	static const char *enames[MAXENGINES+1] = { NULL };

	if (enames[0] == NULL && engines[0].name != NULL) {
		int i;
		for (i=0; i < MAXENGINES && engines[i].name; i++)
			enames[i] = engines[i].name;
		enames[i] = NULL;
	}

	return enames;
}

qlFlags
ql_engine_get_flags(const char *eng)
{
	int i;
	for (i=0; engines[i].name; i++)
		if (!strcmp(eng, engines[i].name))
			return engines[i].flags;
	return QL_FLAG_NONE;
}

qlState *
ql_state_init(const char *eng, qlFlags flags, qlFunction *func, size_t size)
{
	return ql_state_init_full(eng, flags, func, size, NULL,
                              int_resize, int_free, NULL);
}

qlState *
ql_state_init_full(const char *eng, qlFlags flags, qlFunction *func,
                   size_t size, void *memory, qlResize *resize, qlFree *free,
                   void *ctx)
{
	const qlStateEngine *engine;
	qlState *state;
	int i;

	if (!func)
		return NULL;

	if ((flags & QL_FLAG_METHOD_COPY) && (flags & QL_FLAG_METHOD_SHIFT))
		flags &= ~QL_FLAG_METHOD_COPY;

	engine = NULL;
	for (i=0 ; engines[i].name ; i++) {
		if (!strcmp(engines[i].name, eng) ||
				(!eng && (engines[i].flags & flags) == flags)) {
			engine = &engines[i];
			break;
		}
	}
	if (!engine)
		return NULL;

	if (!(flags & (QL_FLAG_METHOD_COPY | QL_FLAG_METHOD_SHIFT))) {
		if (engine->flags & QL_FLAG_METHOD_SHIFT)
			flags |= QL_FLAG_METHOD_SHIFT;
		else if (engine->flags & QL_FLAG_METHOD_COPY)
			flags |= QL_FLAG_METHOD_COPY;
		else
			assert(0);
	}

	/* Make sure we at least have our minimum stack */
	if (!memory || size < engine->size()) {
		if (!resize)
			return NULL;

		size = engine->size();
		state = (*resize)(ctx, memory, size);
		if (!state)
			return NULL;
	} else
		state = memory;

	state->eng    = engine;
	state->flags  = flags;
	state->func   = func;
	state->resize = resize;
	state->free   = free;
	state->ctx    = ctx;
	state->size   = size;

	state->eng->init(state);
	return state;
}

int
ql_state_step(qlState **state, qlParameter* param)
{
	if (!state || !*state)
		return STATUS_ERROR;
	if ((*state)->func) {
		if (!param)
			return STATUS_ERROR;
		(*state)->param = param;
	}

	return (*state)->eng->step(state, param);
}

int
ql_state_yield(qlState **state, qlParameter* param)
{
	if (!state || !*state || !param)
		return STATUS_ERROR;
	if (!(*state)->param)
		return STATUS_CANCEL;

	(*state)->func = NULL;
	*(*state)->param = *param;
	return (*state)->eng->yield(state, param);
}
