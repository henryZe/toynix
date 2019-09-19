#include <lib.h>

void
usage(void)
{
	cprintf("usage: pwd\n");
	exit();
}

void
umain(int argc, char **argv)
{
	if (argc != 1)
		usage();

	printf("%s\n", thisenv->currentpath);
}