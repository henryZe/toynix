/*
 *  kernel/trapentry.S
 *
 *  Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the MIT License.
 */

#include <memlayout.h>
#include <trap.h>

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)								\
	.global name;			/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;				/* align function definition */		\
	name:					/* function starts here */			\
		cli;												\
		pushl $(num);		/* push trap num */				\
		jmp alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)	\
	.global name;					\
	.type name, @function;			\
	.align 2;						\
	name:							\
		cli; 						\
		pushl $0;					\
		pushl $(num);				\
		jmp alltraps

.text
/*
 * Generating entry points for the different traps.
 */
	TRAPHANDLER_NOEC(traphandler_0, T_DIVIDE);
	TRAPHANDLER_NOEC(traphandler_1, T_DEBUG);
	TRAPHANDLER_NOEC(traphandler_2, T_NMI);
	TRAPHANDLER_NOEC(traphandler_3, T_BRKPT);
	TRAPHANDLER_NOEC(traphandler_4, T_OFLOW);
	TRAPHANDLER_NOEC(traphandler_5, T_BOUND);
	TRAPHANDLER_NOEC(traphandler_6, T_ILLOP);
	TRAPHANDLER_NOEC(traphandler_7, T_DEVICE);
	TRAPHANDLER(traphandler_8, T_DBLFLT);
	TRAPHANDLER(traphandler_10, T_TSS);
	TRAPHANDLER(traphandler_11, T_SEGNP);
	TRAPHANDLER(traphandler_12, T_STACK);
	TRAPHANDLER(traphandler_13, T_GPFLT);
	TRAPHANDLER(traphandler_14, T_PGFLT);
	TRAPHANDLER_NOEC(traphandler_16, T_FPERR);
	TRAPHANDLER(traphandler_17, T_ALIGN);
	TRAPHANDLER_NOEC(traphandler_18, T_MCHK);
	TRAPHANDLER_NOEC(traphandler_19, T_SIMDERR);
	TRAPHANDLER_NOEC(traphandler_48, T_SYSCALL);
	TRAPHANDLER_NOEC(irqhandler_0, IRQ_OFFSET + IRQ_TIMER);
	TRAPHANDLER_NOEC(irqhandler_1, IRQ_OFFSET + IRQ_KBD);
	TRAPHANDLER_NOEC(irqhandler_4, IRQ_OFFSET + IRQ_SERIAL);
	TRAPHANDLER_NOEC(irqhandler_7, IRQ_OFFSET + IRQ_SPURIOUS);
	TRAPHANDLER_NOEC(irqhandler_14, IRQ_OFFSET + IRQ_IDE);
	TRAPHANDLER_NOEC(irqhandler_19, IRQ_OFFSET + IRQ_ERROR);

alltraps:
/*
 * generate struct Trapframe
 *
 * +--------------------+ KSTACKTOP
 * | 0x00000 | old SS   |     " - 4
 * |      old ESP       |     " - 8
 * |     old EFLAGS     |     " - 12
 * | 0x00000 | old CS   |     " - 16
 * |      old EIP       |     " - 20
 * |     error code     |     " - 24 <---- ESP
 * +--------------------+
 * |      trap num      |     " - 28
 * +--------------------+
 */
	pushl %ds
	pushl %es
	pushal

	/* Switch to kernel data segment */
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es

	/* pass the argument Trapframe pointer */
	pushl %esp
	call trap

1:	jmp 1b
