#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 16, 0x90
#endif

#define ENTRY(x) \
	.text; _ALIGN_TEXT; .globl x; .type x,@function; x:

ENTRY(toynix_setjmp)
	movl	4(%esp), %ecx		// toynix_jmp_buf

	movl	0(%esp), %edx		// return %eip as pushed by call
	movl	%edx, 0(%ecx)

	leal	4(%esp), %edx		// stack address where %esp will point when we return (leal: load effective address)
	movl	%edx, 4(%ecx)

	movl	%ebp, 8(%ecx)
	movl	%ebx, 12(%ecx)
	movl	%esi, 16(%ecx)
	movl	%edi, 20(%ecx)

	movl	$0, %eax			// return 0
	ret

ENTRY(toynix_longjmp)
	// %eax is the arg1 (toynix_jmp_buf *)
	// %edx is the arg2 (1)

	movl	0(%eax), %ecx		// %eip
	movl	4(%eax), %esp
	movl	8(%eax), %ebp
	movl	12(%eax), %ebx
	movl	16(%eax), %esi
	movl	20(%eax), %edi

	movl	%edx, %eax
	jmp		*(%ecx)
