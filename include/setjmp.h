#ifndef TOYNIX_INC_SETJMP_H
#define TOYNIX_INC_SETJMP_H

#include <arch/i386/setjmp.h>
#include <compiler_attributes.h>

int toynix_setjmp(volatile struct toynix_jmp_buf *buf);
void __noreturn toynix_longjmp(volatile struct toynix_jmp_buf *buf, int val)
	__attribute__((TOYNIX_LONGJMP_GCCATTR));

#endif
