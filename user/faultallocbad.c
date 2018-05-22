#include <lib.h>

void
handler(struct UTrapframe *utf)
{
	int ret;
	void *addr = (void *)utf->utf_fault_va;

	cprintf("fault %x\n", addr);
	ret = sys_page_alloc(0, ROUNDDOWN(addr, PGSIZE), PTE_W);
	if (ret < 0)
		panic("allocating at %x in page fault handler: %e", addr, ret);

	snprintf((char *)addr, 100, "this string was faulted in at %x", addr);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
	/* In fact, it would trap in kernel instead of user space */
	sys_cputs((char *)0xDEADBEEF, 4);
}
