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

#if defined(_WIN32)
#define __NAME(name) _##name
#define __TYPE(name) .def _##name; .scl 2; .type 32; .endef
#elif defined(__APPLE__)
#define __NAME(name) _##name
#define __TYPE(name) .align 4, 0x90
#else
#define __NAME(name) name
#define __TYPE(name) .type name, @function
#endif

#if defined(__GCC_HAVE_DWARF2_CFI_ASM)
#define __STARTPROC .cfi_startproc
#define __ENDPROC .cfi_endproc
#define __DEBUG .cfi_sections .debug_frame
#elif defined(__APPLE__)
#define __CAT_(x, y) x##y
#define __CAT(x, y) __CAT_(x, y)
#define __STARTPROC __CAT(Leh_func_begin, __LINE__):
#define __ENDPROC __CAT(Leh_func_end, __LINE__):
#define __DEBUG
#else
#define __STARTPROC
#define __ENDPROC
#define __DEBUG
#endif

#if defined(__x86_64__) || defined(__x86_64) || \
    defined(__amd64__)  || defined(__amd64)  || \
    defined(_M_X64)
#define QL64
#define PSIZE	8
#define AX	%rax
#define SP	%rsp
#define ARG0	%rdi
#define ARG1	%rsi
#define ARG2	%rdx
#define ARG3	%rcx
#else
#define PSIZE	4
#define AX	%eax
#define SP	%esp
#define ARG0	4(SP)
#define ARG1	8(SP)
#define ARG2	12(SP)
#define ARG3	16(SP)
#endif

/*
 * int get_stack_direction(void);
 */
                .text
                .globl  __NAME(get_stack_direction)
                __DEBUG
                __TYPE(get_stack_direction)
__NAME(get_stack_direction):
        __STARTPROC
        mov     $-1,            AX
        ret
        __ENDPROC

/*
 * uintptr_t get_stack(void);
 */
                .text
                .globl  __NAME(get_stack)
                __DEBUG
                __TYPE(get_stack)
__NAME(get_stack):
        __STARTPROC
        lea     PSIZE(SP),	AX
        ret
        __ENDPROC

/*
 * qlParameter *
 * translate(uintptr_t dst, uintptr_t src, void *buf, qlParameter *ptr);
 */
                .text
                .globl  __NAME(translate)
                __DEBUG
                __TYPE(translate)
__NAME(translate):
        __STARTPROC
        mov     ARG3,		AX

        /* We do not do translation if:
         *   1. dst == 0   (QL_METHOD_SHIFT is being used)
         *   2. ptr <  dst (ptr is not in the copy buffer)
         *   3. ptr >= src (ptr is not in the copy buffer)
         */
        cmp	$0,		ARG0
        je	return
        cmp	ARG0,		AX
        jl	return
        cmp	ARG1,		AX
        jge	return

        sub	ARG0,		AX
        add	ARG2,		AX
return:
        ret
        __ENDPROC

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
#ifdef QL64
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
#else
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
#endif
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
#ifdef QL64
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
#else
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
#endif
	call	__NAME(dolongjmp)
	__ENDPROC
