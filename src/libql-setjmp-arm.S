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

/*
 * void CCONV
 * call_function(qlStateSetJmp *state, qlParameter *param,
 *               qlFunction *func, void *stack, size_t size, jmp_buf buf);
 */
		.text
		.globl  call_function
		.type   call_function, %function
call_function:
	.cfi_startproc
	/* Load the last parameters into registers */
	ldr	r4,	[sp]
	ldr	r5,	[sp, #4]

	/* Set the new stack; push important registers */
	add	sp,	r3,	r4
	push    {r5}
	push	{r1}

	/* Do the call */
	ldr	r1,	[r1] /* Dereference param */
	blx	r2

	/* Set the parameter */
	pop	{r1}
	str	r0,	[r1]

	/* Jump */
	pop	{r0}
	mov	r1,	#STATUS_RETURN
	bl	dolongjmp
	.cfi_endproc
