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

/*
 * qlStateAssembly *CCONV
 * call_function(qlStateAssembly *state, qlParameter *param,
 *               qlFunction *func, void *stack, size_t size);
 */
		.text
		.globl  call_function
		.type   call_function, %function
call_function:
	.cfi_startproc

	/* Set the new stack; store srcbuf and param on the new stack */
	push	{r4, r5}
	ldr	r4,	[sp] /* Load size into r4 */
	mov	r5,	sp   /* Copy old stack into r5 */
	add	sp,	r3,	r4
	push    {r0}
	push	{r1, r5, lr}

	/* Do the call */
	ldr	r1,	[r1] /* Dereference param */
	blx	r2
	pop	{r1, r5, lr}

	/* Set the parameter */
	str	r0,	[r1]

	/* Return the state */
	pop	{r0}

	/* Restore the stack */
	mov	sp,	r5
	pop	{r4, r5}

	/* Return */
	bx	lr
	.cfi_endproc
