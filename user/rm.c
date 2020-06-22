#include <lib.h>

static void
usage(void)
{
	cprintf("rm FILE...\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int f, i;

	if (argc == 1)
		usage();

	for (i = 1; i < argc; i++) {
		f = remove(argv[i]);
		if (f < 0)
			printf("can't remove %s: %e\n", argv[i], f);
	}
}