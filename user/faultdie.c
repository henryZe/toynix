#include <lib.h>

void
handler(struct UTrapframe *utf)
{
	void *addr = (void *)utf->utf_fault_va;
	uint32_t err = utf->utf_err;

	cprintf("i faulted at va %x, err %x\n", addr, err & 7);
	sys_env_destroy(0);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(handler);
	*(int *)0xDeadBeef = 0;
}