#include <lib.h>

void
umain(int argc, char **argv)
{
	int i, test[4096];

	cprintf("i am environment %08x %p\n",
		thisenv->env_id, thisenv->env_pgfault_upcall);

	for (i = 0; i < 4096; i++)
		test[i] = i;

	cprintf("test[100] %d\n", test[4095]);
}
