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
	.ent	call_function
	.type   call_function, @function
call_function:
	.frame	$sp,	32,	$31
	.set 	noreorder

	/* Load the last parameters into registers */
	lw	$t0,	16($sp)
	lw	$t1,	20($sp)

	/* Set the new stack; push important registers */
	add     $sp,	$a3,	$t0
	addi	$sp,	$sp,	-32
	.cprestore	28
	sw	$t1,	24($sp)
	sw	$a1,	20($sp)
	sw	$gp,	16($sp)

	/* Do the call : func(state, *param) */
	lw	$a1,	($a1)
	move	$25,	$a2
	jal	$25
	nop
	lw	$gp,	16($sp)

	/* Pop param, save the function's return value */
	lw	$a1,	20($sp)
	sw	$v0,	($a1)

	/* Jump */
	lw	$a0,	24($sp)
	li	$a1,	STATUS_RETURN
	jal	dolongjmp
	nop

	.set	reorder
	.end	call_function
	.size	call_function,	.-call_function
