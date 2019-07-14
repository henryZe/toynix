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
	binaryname = "pwd";

	if (argc != 1)
		usage();

	printf("%s\n", thisenv->currentpath);
}