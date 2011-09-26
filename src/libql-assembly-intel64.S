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
 * void
 * resume_function(jmp_buf dstbuf, jmp_buf srcbuf, uintptr_t dst,
 *                 uintptr_t src, void *buf, qlParameter *param);
 */
		.text
		.globl  __NAME(resume_function)
		__DEBUG
		__TYPE(resume_function)
__NAME(resume_function):
	__STARTPROC

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

 	/* longjmp(dstbuf, sparam ? STATUS_RESUME : STATUS_CANCEL); */
 	cmp	$0,		%r9
 	jne	statusresume
 	jmp	statuscancel
statusresume:
	mov	$STATUS_RESUME,	%rsi
	jmp	dojmp
statuscancel:
	mov	$STATUS_CANCEL,	%rsi
	jmp	dojmp
dojmp:
 	call	__NAME(dolongjmp)
	__ENDPROC

/*
 * void
 * call_function(qlState **state, qlParameter *param, qlFunction *func,
 *               uintptr_t stack, jmp_buf srcbuf);
 */
		.text
		.globl  __NAME(call_function)
		__DEBUG
		__TYPE(call_function)
__NAME(call_function):
	__STARTPROC

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
	call	__NAME(dolongjmp)
	__ENDPROC
