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
#define AX	%rax
#define BX	%rbx
#define CX	%rcx
#define DX	%rdx
#define DI	%rdi
#define SI	%rsi
#define BP	%rbp
#define SP	%rsp
#define ARG4	%r8
#else
#define AX	%eax
#define BX	%ebx
#define CX	%ecx
#define DX	%edx
#define DI	%edi
#define SI	%esi
#define BP	%ebp
#define SP	%esp
#define ARG4	20(SP)
#endif

/*
 * qlStateAssembly *CCONV
 * call_function(qlStateAssembly *state, qlParameter *param,
 *               qlFunction *func, void *stack, size_t size);
 */
		.text
		.globl  __NAME(call_function)
		__DEBUG
		__TYPE(call_function)
__NAME(call_function):
	__STARTPROC
	pop	BX /* Get the return address */
	push	BP
	mov	SP,		BP

#ifndef QL64
	mov	4(%ebp),	%edi
	mov	8(%ebp),	%esi
	mov	12(%ebp),	%edx
	mov	16(%ebp),	%ecx
#endif

	/* Set new stack; store old stack, state and param on the new stack */
	add	ARG4,		CX
	mov	CX,		SP
	push	BP /* old stack */
	push	BX /* return address */
	push    DI /* state */
	push	SI /* param */
	mov	(SI),		SI /* dereference param */

	/* Call the function */
#ifdef QL64
	call	*%rdx
#else
	push	%esi /* *param */
	push	%edi /* state */
	call	*%edx
	add	$8,		%esp
#endif

	/* Save the parameter and restore stack */
	pop	DX /* param */
	mov	AX,		(DX)
	pop	AX /* state */
	pop	BX /* return address */
	pop	SP /* old stack */

	/* Return */
	pop	BP
	jmp	*BX
	__ENDPROC
