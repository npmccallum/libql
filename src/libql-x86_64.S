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
		.type   get_stack_direction, @function
get_stack_direction:
	.cfi_startproc
	push	%rsp
	mov	%rsp,		%rax
	pop	%rsp
	sub	%rsp,		%rax
	ret
	.cfi_endproc

/*
 * uintptr_t get_stack(void);
 */
		.text
		.globl  get_stack
		.type   get_stack, @function
get_stack:
	.cfi_startproc
	lea	8(%rsp),	%rax
	ret
	.cfi_endproc

/*
 * qlParameter *
 * translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);
 */
		.text
		.globl  translate
		.type   translate, @function
translate:
	.cfi_startproc
	mov	%rcx,		%rax

	/* We do not do translation if:
	 *   1. dst == 0   (QL_METHOD_SHIFT is being used)
	 *   2. ptr <  dst (ptr is not in the copy buffer)
	 *   3. ptr >= src (ptr is not in the copy buffer)
	 */
	cmp	$0,		%rdi
	je	return
	cmp	%rdi,		%rax
	jl	return
	cmp	%rsi,		%rax
	jge	return

	sub	%rdi,		%rax
	add	%rdx,		%rax
return:
	ret
	.cfi_endproc

/*
 * void
 * resume_function(jmp_buf dstbuf, jmp_buf srcbuf, uintptr_t dst,
 *                 uintptr_t src, void *buf, qlParameter *param);
 */
		.text
		.globl  resume_function
		.type   resume_function, @function
resume_function:
	.cfi_startproc

	/* If no buffer is specified, than we won't do a copy */
	cmp	$0,		%r8
	je	nocopy

	/* Push out the stack */
	mov	%rdx,		%rsp

	/* memcpy(dst, buf, src-dst); */
	push	%rcx
	push	%rdi
	push	%rsi
	mov	%r8,		%rsi
	mov	%rdx,		%rdi
	sub	%rdx,		%rcx
	rep	movsb
	pop	%rsi
	pop	%rdi
	pop	%rcx

nocopy:
	/* Save the src jmp_buf and the param on the stack.
	 * See call_function(). */
	mov	%rsi,		-8(%rcx)
	mov	%r9,		-16(%rcx)

 	/* longjmp(dstbuf, sparam ? 1 : -1); */
 	cmp	$0,		%r9
 	jne	posone
 	jmp	negone
posone:
	mov	$1,		%rsi
	jmp	dojmp
negone:
	mov	$-1,		%rsi
	jmp	dojmp
dojmp:
 	call	dolongjmp
	.cfi_endproc

/*
 * void
 * call_function(qlState **state, qlParameter *param, qlFunction *func,
 *               uintptr_t stack, jmp_buf srcbuf);
 */
		.text
		.globl  call_function
		.type   call_function, @function
call_function:
	.cfi_startproc

	/* Set the new stack; store srcbuf and param on the new stack */
	mov	%rcx,		%rsp
	push	%r8
	push	%rsi

	/* Do the call */
	mov	(%rsi),		%rsi
	call	*%rdx

	/* Set the return value and jump */
	pop	%rsi /* This may have changed, see resume_function() */
	mov	%rax,		(%rsi)

	/* Jump */
	pop	%rdi /* This may have changed, see resume_function() */
	mov	$1,		%rsi
	call	dolongjmp
	.cfi_endproc
