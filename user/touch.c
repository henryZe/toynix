#include <lib.h>

void
usage(void)
{
	cprintf("touch FILE...\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int f, i;

	if (argc == 1)
		usage();

	for (i = 1; i < argc; i++) {
		f = open(argv[i], O_CREAT);
		if (f < 0)
			printf("can't open %s: %e\n", argv[i], f);
		else
			close(f);
	}
}