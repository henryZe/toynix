#ifndef _ALIGN_TEXT
#define _ALIGN_TEXT .align 16, 0x90
#endif

#define ENTRY(x) \
	.text; _ALIGN_TEXT; .globl x; .type x,@function; x:

/*
 * struct toynix_jmp_buf {
 *   uint32_t jb_eip;
 *   uint32_t jb_esp;
 *   uint32_t jb_ebp;
 *   uint32_t jb_ebx;
 *   uint32_t jb_esi;
 *   uint32_t jb_edi;
 * };
 */

ENTRY(toynix_setjmp)
	movl	4(%esp), %ecx		// arg1 toynix_jmp_buf -> ecx

	movl	0(%esp), %edx		// return %eip as pushed by call
	movl	%edx, 0(%ecx)		// eip -> jb_eip

	leal	4(%esp), %edx		// stack address where (%esp + 4) will point when we ret (leal: load effective address)
	movl	%edx, 4(%ecx)		// esp -> jb_esp

	movl	%ebp, 8(%ecx)		// ebp -> jb_ebp
	movl	%ebx, 12(%ecx)		// ebx -> jb_ebx
	movl	%esi, 16(%ecx)		// esi -> jb_esi
	movl	%edi, 20(%ecx)		// edi -> jb_edi

	movl	$0, %eax		// return 0
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

	movl	%edx, %eax		// return value
	jmp	*%ecx
