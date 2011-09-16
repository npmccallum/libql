/*
 * int get_stack_direction(void);
 */
		.text
		.globl  get_stack_direction
		.type   get_stack_direction, @function
get_stack_direction:
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
		.globl  get_stack
		.type   get_stack, @function
get_stack:
	.cfi_startproc
	lea	4(%esp),	%eax
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
		.globl  resume_function
		.type   resume_function, @function
resume_function:
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

 	/* longjmp(dstbuf, sparam ? 1 : -1); */
 	cmpl	$0,		24(%ebp)
 	jne	posone
 	jmp	negone
posone:
	pushl	$1
	jmp	dojmp
negone:
	pushl	$-1
	jmp	dojmp
dojmp:
	mov	4(%ebp),	%eax
	push	%eax
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
	call	dolongjmp
	.cfi_endproc
