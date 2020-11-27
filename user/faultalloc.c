#include <lib.h>

static void
handler(struct UTrapframe *utf)
{
	int ret;
	void *addr = (void *)utf->utf_fault_va;

	cprintf("fault %p\n", addr);
	ret = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE), PTE_W);
	if (ret < 0)
		panic("allocating at %x in page fault handler: %e", addr, ret);

	snprintf((char *)addr, 100, "this string was faulted in at %p", addr);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
	cprintf("%s\n", (char *)0xDeadBeef);
	cprintf("%s\n", (char *)0xCafeBffe);
}
