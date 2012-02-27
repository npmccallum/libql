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
	.ent	call_function
	.frame	$sp,	0,	$ra

	/* Set the new stack */
	lw	$t0,	16($sp) /* Load size into t0 */
	add     $sp,	$a3,	$t0

	/* Push return address, state and param on the new stack */
	addi	$sp,	$sp,	-12
	sw	$ra,	8($sp)
	sw	$a0,	4($sp)
	sw	$a1,	($sp)

	/* Do the call */
	lw	$a1,	($a1)
	jal	$a2

	/* Pop param, save the function's return value */
	lw	$a1,	($sp)
	sw	$v0,	($a1)

	/* Pop state and return address and return */
	lw	$v0,	4($sp)
	lw	$ra,	8($sp)
	addi	$sp,	$sp,	12
	jr	$ra
	.end	call_function
	.size	call_function,	.-call_function
