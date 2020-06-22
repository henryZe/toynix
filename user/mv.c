#include <lib.h>

static void
usage(void)
{
	cprintf("usage: mv SOURCE DIRECTORY\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int ret;
	char *src, *dst;

	if (argc != 3)
		usage();

	src = argv[1];
	dst = argv[2];

	ret = rename(src, dst);
	if (ret < 0)
		printf("rename %s %s failed: %e", src, dst, ret);
}