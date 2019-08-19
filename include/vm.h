#ifndef INC_VM_H
#define INC_VM_H

#include <trap.h>

#define VMA_PER_ENV 8

struct vm_area_struct {
	unsigned long vm_start;
	uint32_t size;
	uint32_t vm_page_prot;
	int fd;
	off_t offset;
	void (*page_fault)(struct UTrapframe *utf);
};

#endif