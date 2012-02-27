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

#define STATUS_RETURN 1
#define STATUS_YIELD  2

#include <libql.h>
#include <stddef.h>

#include <libsc.h>

typedef struct qlStateEngine qlStateEngine;

struct qlState {
  const qlStateEngine *eng;
  qlFunction         *func;
  qlParameter        param;
  void              *stack;
};

#endif /* LIBQL_INTERNAL_H_ */
