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

#ifdef _WIN32
#define __NAME(name) _##name
#define __TYPE(name) .def _##name; .scl 2; .type 32; .endef
#else
#define __NAME(name) name
#define __TYPE(name) .type name, @function
#endif

/*
 * int get_stack_direction(void);
 */
		.text
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
		.cfi_sections .debug_frame
#endif
		.globl	__NAME(get_stack_direction)
		__TYPE(get_stack_direction)
__NAME(get_stack_direction):
	.cfi_startproc
	push	%esp
	mov	%esp,		%eax
	pop	%esp
	sub	%esp,		%eax
	ret
	.cfi_endproc

/*
 * uintptr_t get_stack(void);
 */
		.text
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
		.cfi_sections .debug_frame
#endif
		.globl	__NAME(get_stack)
		__TYPE(get_stack)
__NAME(get_stack):
	.cfi_startproc
	lea	4(%esp),	%eax
	ret
	.cfi_endproc

/*
 * qlParameter *
 * translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);
 */
		.text
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
		.cfi_sections .debug_frame
#endif
		.globl	__NAME(translate)
		__TYPE(translate)
__NAME(translate):
	.cfi_startproc
	/* Put the pointer under consideration into eax */
	mov	16(%esp),	%eax

	/* We do not do translation if:
	 *   1. dst == 0   (QL_METHOD_SHIFT is being used)
	 *   2. ptr <  dst (ptr is not in the copy buffer)
	 *   3. ptr >= src (ptr is not in the copy buffer)
	 */
	cmpl	$0,		4(%esp)
	je	return
	cmp	4(%esp),	%eax
	jl	return
	cmp	8(%esp),	%eax
	jge	return

	sub	4(%esp),	%eax
	add	12(%esp),	%eax
return:
	ret
	.cfi_endproc

/*
 * void
 * resume_function(jmp_buf dstbuf, jmp_buf srcbuf, uintptr_t dst,
 *                 uintptr_t src, void *buf, qlParameter *param);
 */
		.text
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
		.cfi_sections .debug_frame
#endif
		.globl	__NAME(resume_function)
		__TYPE(resume_function)
__NAME(resume_function):
	.cfi_startproc
	mov	%esp,		%ebp

	/* If no buffer is specified, than we won't do a copy */
	cmpl	$0,		20(%ebp)
	je	nocopy

	/* Push out the stack */
	mov	12(%ebp),	%esp

	/* memcpy(dst, buf, src-dst); */
	mov	20(%ebp),	%esi
	mov	12(%ebp),	%edi
	mov	16(%ebp),	%ecx
	sub	12(%ebp),	%ecx
	rep	movsb

nocopy:
	/* Save the src jmp_buf and the param on the stack.
	 * See call_function(). */
	mov	16(%ebp),	%ecx
	mov	8(%ebp),	%eax     /* Copy ...... */
	mov	%eax,		-4(%ecx) /*      srcbuf */
	mov	24(%ebp),	%eax     /* Copy ...... */
	mov	%eax,		-8(%ecx) /*      param  */

 	/* longjmp(dstbuf, sparam ? STATUS_RESUME : STATUS_CANCEL); */
 	cmpl	$0,		24(%ebp)
 	jne	statusresume
 	jmp	statuscancel
statusresume:
	pushl	$STATUS_RESUME
	jmp	dojmp
statuscancel:
	pushl	$STATUS_CANCEL
	jmp	dojmp
dojmp:
	mov	4(%ebp),	%eax
	push	%eax
 	call	__NAME(dolongjmp)
	.cfi_endproc

/*
 * void
 * call_function(qlState **state, qlParameter *param, qlFunction *func,
 *               uintptr_t stack, jmp_buf srcbuf);
 */
		.text
#ifdef __GCC_HAVE_DWARF2_CFI_ASM
		.cfi_sections .debug_frame
#endif
		.globl	__NAME(call_function)
		__TYPE(call_function)
__NAME(call_function):
	.cfi_startproc
	mov	%esp,		%ebp

	/* Set the new stack; store srcbuf and param on the new stack */
	mov	16(%ebp),	%esp
	mov	20(%ebp),	%eax
	push	%eax
	mov	8(%ebp),	%eax
	push	%eax

	/* Do the call */
	mov	(%eax),		%eax
	push	%eax
	mov	4(%ebp),	%eax
	push	%eax
	mov	12(%ebp),	%eax
	call	*%eax
	add	$8,		%esp

	/* Set the return value and jump */
	pop	%edx /* This may have changed, see resume_function() */
	mov	%eax,		(%edx)

	/* Jump */
	pop	%edx /* This may have changed, see resume_function() */
	mov	$1,		%eax
	push	%eax
	push	%edx
	call	__NAME(dolongjmp)
	.cfi_endproc
