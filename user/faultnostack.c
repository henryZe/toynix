#include <lib.h>

void
umain(int argc, char **argv)
{
	sys_env_set_pgfault_upcall(0, _pgfault_upcall);
	*(int *)0 = 0;
}
