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
#define PSIZE	8
#define AX	%rax
#define SP	%rsp
#define ARG0	%rdi
#define ARG1	%rsi
#define ARG2	%rdx
#define ARG3	%rcx
#include "libql-assembly-intel64.S"
#else
#define PSIZE	4
#define AX	%eax
#define SP	%esp
#define ARG0	4(SP)
#define ARG1	8(SP)
#define ARG2	12(SP)
#define ARG3	16(SP)
#include "libql-assembly-intel32.S"
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
