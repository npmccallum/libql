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

#ifndef LIBQL_INTERNAL_H_
#define LIBQL_INTERNAL_H_
#ifndef __ASSEMBLER__
#include <libql.h>
#include <stddef.h>
#endif /* __ASSEMBLER__ */

#define STATUS_CANCEL -1
#define STATUS_OK      0
#define STATUS_ERROR   1
#define STATUS_RESUME  3

#ifndef __ASSEMBLER__
typedef struct qlStateEngine qlStateEngine;

struct qlState {
	const qlStateEngine *eng;
	qlFlags            flags;
	qlFunction         *func;
	qlParameter       *param;
	qlResize         *resize;
	qlFree	           *free;
	void                *ctx;
	size_t              size;
};

size_t
get_pagesize();
#endif /* __ASSEMBLER__ */
#endif /* LIBQL_INTERNAL_H_ */
