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
 * int get_stack_direction(void);
 */
		.text
		.globl  get_stack_direction
		.type   get_stack_direction, %function
get_stack_direction:
	.cfi_startproc
	mov	r0,	#-1
	bx	lr
	.cfi_endproc

/*
 * uintptr_t get_stack(void);
 */
		.text
		.globl  get_stack
		.type   get_stack, %function
get_stack:
	.cfi_startproc
	mov	r0,	sp
	bx	lr
	.cfi_endproc

/*
 * qlParameter *
 * translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);
 */
		.text
		.globl  translate
		.type   translate, %function
translate:
	.cfi_startproc

	/* We do not do translation if:
	 *   1. dst == 0   (QL_METHOD_SHIFT is being used)
	 *   2. ptr <  dst (ptr is not in the copy buffer)
	 *   3. ptr >= src (ptr is not in the copy buffer)
	 */
	cmp	r0,	#0
	beq	.Lreturn
	cmp	r3,	r0
	blt	.Lreturn
	cmp	r3,	r1
	bge	.Lreturn

	sub	r3,	r3,	r0
	add	r3,	r3,	r2
.Lreturn:
	mov	r0,	r3
	bx	lr
	.cfi_endproc

/*
 * void
 * resume_function(jmp_buf dstbuf, jmp_buf srcbuf, uintptr_t dst,
 *                 uintptr_t src, void *buf, qlParameter *param);
 */
		.text
		.globl  resume_function
		.type   resume_function, %function
resume_function:
	.cfi_startproc
	ldr	r4,	[sp]
	ldr	r5,	[sp, #4]

	/* If no buffer is specified, than we won't do a copy */
	cmp	r4,	#0
	beq	.Lnocopy

	/* Push out the stack */
	mov	sp,	r2

	/* memcpy(dst, buf, src-dst); */
	push	{r0-r10}
	sub	r0,	r3,	r2

.Lcpy32:
	cmp	r0,	#32
	blt	.Lcpy16
	ldmia	r4!,	{r1, r3, r5-r10}
	stmia	r2!,	{r1, r3, r5-r10}
	sub	r0,	r0,	#32
	bl	.Lcpy32

.Lcpy16:
	cmp	r0,	#16
	blt	.Lcpy8
	ldmia	r4!,	{r1, r3, r5-r6}
	stmia	r2!,	{r1, r3, r5-r6}
	sub	r0,	r0,	#16

.Lcpy8:
	cmp	r0,	#8
	blt	.Lcpy1
	ldmia	r4!,	{r1, r3}
	stmia	r2!,	{r1, r3}
	sub	r0,	r0,	#8

.Lcpy1:
	cmp	r0,	#1
	blt	.Lcpydone
	ldrb	r1,	[r4]
	strb	r1,	[r2]
	add	r2,	r2,	#1
	add	r4,	r4,	#1
	sub	r0,	r0,	#1
	bl	.Lcpy1

.Lcpydone:
	pop	{r0-r10}

.Lnocopy:
	/* Save the src jmp_buf and the param on the stack.
	 * See call_function(). */
	str	r1,	[r3, #-4]
	str	r5,	[r3, #-8]

 	/* longjmp(dstbuf, sparam ? 1 : -1); */
	mov	r1,	#-1
 	cmp	r5,	#0
	addne	r1,	r1,	#2
 	bl	dolongjmp
	.cfi_endproc

/*
 * void
 * call_function(qlState **state, qlParameter *param, qlFunction *func,
 *               uintptr_t stack, jmp_buf srcbuf);
 */
		.text
		.globl  call_function
		.type   call_function, %function
call_function:
	.cfi_startproc
	ldr	r4,	[sp] /* Load srcbuf into r4 */

	/* Set the new stack; store srcbuf and param on the new stack */
	mov	sp,	r3
	push	{r1, r4}

	/* Do the call */
	ldr	r1,	[r1]
	blx	r2
	pop	{r1, r4} /* This may have changed, see resume_function() */

	/* Set the return value */
	str	r0,	[r1]

	/* Jump */
	mov	r0,	r4
	mov	r1,	#1
	bl	dolongjmp
	.cfi_endproc
