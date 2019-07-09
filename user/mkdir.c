#include <lib.h>

void
usage(void)
{
	cprintf("mkdir DIRECTORY...\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int ret, i;

	binaryname = "mkdir";

	if (argc == 1)
		usage();

	for (i = 1; i < argc; i++) {
		ret = mkdir(argv[i]);
		if (ret < 0)
			printf("can't create directory %s: %e\n", argv[i], ret);
	}
}