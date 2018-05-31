#include <lib.h>

void
umain(int argc, char **argv)
{
	sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), PTE_W);
	sys_env_set_pgfault_upcall(0, (void *)0xDeadBeef);
	*(int *)0 = 0;
}
