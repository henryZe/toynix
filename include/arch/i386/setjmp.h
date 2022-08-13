#ifndef TOYNIX_MACHINE_SETJMP_H
#define TOYNIX_MACHINE_SETJMP_H

#include <types.h>

#define TOYNIX_LONGJMP_GCCATTR	regparm(2)

struct toynix_jmp_buf {
	uint32_t jb_eip;
	uint32_t jb_esp;
	uint32_t jb_ebp;
	uint32_t jb_ebx;
	uint32_t jb_esi;
	uint32_t jb_edi;
};

#endif
