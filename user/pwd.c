#include <lib.h>

static void
usage(void)
{
	cprintf("%s: pwd\n", __func__);
	exit();
}

void
umain(int argc, char **argv)
{
	if (argc != 1)
		usage();

	printf("%s\n", thisenv->currentpath);
}
