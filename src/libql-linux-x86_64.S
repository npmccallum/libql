#define MSC	0
#define STK	8

#define RBX	0
#define RCX	1
#define RDX	2
#define RBP	3
#define RSP	4
#define RSI	5
#define R8	6
#define R9	7
#define R10	8
#define R11	9
#define R12	10
#define R13	11
#define R14	12
#define R15	13
#define RET	14

#define REGISTERS 15

#define ORIG(r)	(r*8+16)
#define JUMP(r)	(r*8+REGISTERS*8+16)
#define TOTAL	JUMP(REGISTERS)

/*
 * size_t int_ql_size(void);
 */
 	.text
	.globl	int_ql_size
	.type	int_ql_size, @function
int_ql_size:
	.cfi_startproc
	movq	$TOTAL,			%rax
	ret
	.cfi_endproc

/*
 * size_t int_ql_current_partial_stack_size(qlState *state);
 */
 	.text
	.globl	int_ql_current_partial_stack_size
	.type	int_ql_current_partial_stack_size, @function
int_ql_current_partial_stack_size:
	.cfi_startproc
	movq	ORIG(RSP)(%rdi),	%rax
	leaq	8(%rsp),		%rdi
	subq	%rdi,			%rax
	ret
	.cfi_endproc

/*
 * int int_ql_call(qlFunction *call, qlState *state, void** misc);
 */
	.text
	.globl	int_ql_call
	.type	int_ql_call, @function
int_ql_call:
	.cfi_startproc
	leaq	8(%rsp),		%rax /* Store the Caller's RSP */

	/* If this is the first time called, we skip the stack check */
	cmpq	$0,			(STK)(%rsi)
	je	storestate

	/* Ensure that we are resuming from the same point in the stack */
	cmpq	%rbp,			ORIG(RBP)(%rsi)
	jne	error
	cmpq	%rax,			ORIG(RSP)(%rsi)
	jne	error

storestate:
	/* Store the state */
	movq	%rbx,			ORIG(RBX)(%rsi)
	movq	%rcx,			ORIG(RCX)(%rsi)
	movq	%rdx,			ORIG(RDX)(%rsi)
	movq	%rbp,			ORIG(RBP)(%rsi) /* Caller's RBP */
	movq	%rax,			ORIG(RSP)(%rsi) /* Caller's RSP */
	movq	%rsi,			ORIG(RSI)(%rsi)
	movq	%r8,			ORIG(R8) (%rsi)
	movq	%r9,			ORIG(R9) (%rsi)
	movq	%r10,			ORIG(R10)(%rsi)
	movq	%r11,			ORIG(R11)(%rsi)
	movq	%r12,			ORIG(R12)(%rsi)
	movq	%r13,			ORIG(R13)(%rsi)
	movq	%r14,			ORIG(R14)(%rsi)
	movq	%r15,			ORIG(R15)(%rsi)
	movq	(%rsp),			%rax      /* Save the return pointer */
	movq	%rax,			ORIG(RET)(%rsi)

	cmpq	$0,			(STK)(%rsi)
	jne	resume

	/* Setup the misc pointer */
	movq	%rdx,			(MSC)(%rsi)

	/* Realign the arguments */
	movq	%rdi,			%rax
	movq	%rsi,			%rdi /* qlState* */
	movq	(%rdx),			%rsi /* Dereference void** */

	/* Call the function */
	pushq	%rdx
	call	*%rax
	popq	%rdx
	movq	%rax,			(%rdx)
	movq	$2,			%rax
	ret

resume:
	movq	%rsi,			%rax

	/* Copy the heap back to the stack */
	movq	JUMP(RSP)(%rax),	%rsp /* Restore the stack pointer */
	movq	(STK)(%rax),		%rsi
	movq	%rsp,			%rdi
	movq	ORIG(RSP)(%rax),	%rcx
	subq	%rsp,			%rcx
	rep	movsb

	/* Setup the misc pointer */
	cmpq	$0,			%rdx
	je	nomisc
	movq	(MSC)(%rax),		%rdi
	movq	(%rdx),			%rsi
	movq	%rsi,			(%rdi)
nomisc:
	movq	%rdx,			(MSC)(%rax) /* Finish pointer setup */

	/* Restore the context */
	movq	JUMP(RBX)(%rax),	%rbx
	movq	JUMP(RCX)(%rax),	%rcx
	movq	JUMP(RDX)(%rax),	%rdx
	movq	JUMP(RBP)(%rax),	%rbp
	  /* RSP already restored above */
	movq	JUMP(RSI)(%rax),	%rsi
	movq	JUMP(R8) (%rax),	%r8
	movq	JUMP(R9) (%rax),	%r9
	movq	JUMP(R10)(%rax),	%r10
	movq	JUMP(R11)(%rax),	%r11
	movq	JUMP(R12)(%rax),	%r12
	movq	JUMP(R13)(%rax),	%r13
	movq	JUMP(R14)(%rax),	%r14
	movq	JUMP(R15)(%rax),	%r15
	movq	JUMP(RET)(%rax),	%rdi

	/* Return in int_ql_yield() */
	movq	$1,			%rax
	jmp	*%rdi

error:
	movq	$0,			%rax
	ret
	.cfi_endproc

/*
 * int int_ql_yield(qlState *state);
 */
	.text
	.globl	int_ql_yield
	.type	int_ql_yield, @function
int_ql_yield:
	.cfi_startproc
	popq	%rax /* Pop the return address */

	/* Store the current context */
	movq	%rbx,			JUMP(RBX)(%rdi)
	movq	%rcx,			JUMP(RCX)(%rdi)
	movq	%rdx,			JUMP(RDX)(%rdi)
	movq	%rbp,			JUMP(RBP)(%rdi) /* Caller's RBP */
	movq	%rsp,			JUMP(RSP)(%rdi) /* Caller's RSP */
	movq	%rsi,			JUMP(RSI)(%rdi)
	movq	%r8,			JUMP(R8) (%rdi)
	movq	%r9,			JUMP(R9) (%rdi)
	movq	%r10,			JUMP(R10)(%rdi)
	movq	%r11,			JUMP(R11)(%rdi)
	movq	%r12,			JUMP(R12)(%rdi)
	movq	%r13,			JUMP(R13)(%rdi)
	movq	%r14,			JUMP(R14)(%rdi)
	movq	%r15,			JUMP(R15)(%rdi)
	movq	%rax,			JUMP(RET)(%rdi)

	/* Copy the stack to the heap */
	movq	%rdi,			%rax
	movq	%rsp,			%rsi
	movq	(STK)(%rax),		%rdi
	movq	ORIG(RSP)(%rax),	%rcx
	subq	%rsp,			%rcx
	rep	movsb

	/* Restore the old context */
	movq	ORIG(RBX)(%rax),	%rbx
	movq	ORIG(RCX)(%rax),	%rcx
	movq	ORIG(RDX)(%rax),	%rdx
	movq	ORIG(RBP)(%rax),	%rbp
	movq	ORIG(RSP)(%rax),	%rsp
	movq	ORIG(RSI)(%rax),	%rsi
	movq	ORIG(R8) (%rax),	%r8
	movq	ORIG(R9) (%rax),	%r9
	movq	ORIG(R10)(%rax),	%r10
	movq	ORIG(R11)(%rax),	%r11
	movq	ORIG(R12)(%rax),	%r12
	movq	ORIG(R13)(%rax),	%r13
	movq	ORIG(R14)(%rax),	%r14
	movq	ORIG(R15)(%rax),	%r15
	movq	ORIG(RET)(%rax),	%rdi

	/* Return in int_ql_call() */
	movq	$1,			%rax
	jmp	*%rdi
	.cfi_endproc
