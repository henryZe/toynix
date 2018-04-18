/*
 *  kernel/trapentry.S
 *
 *  Copyright (C) 2018 Henry.Zeng <henryz_e@163.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the MIT License.
 */

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
#define TRAPHANDLER(name, num)										\
	.global name;			/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */			\
	.align 2;				/* align function definition */			\
	name:					/* function starts here */				\
		pushl $(num);												\
		jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)	\
	.global name;					\
	.type name, @function;			\
	.align 2;						\
	name:							\
		pushl $0;					\
		pushl $(num);				\
		jmp _alltraps

.text

/*
 * Generating entry points for the different traps.
 */

_alltraps:



