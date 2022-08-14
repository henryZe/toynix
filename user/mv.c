#include <lib.h>

static void
usage(void)
{
	cprintf("%s: mv SOURCE DIRECTORY\n", __func__);
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
		warn("rename %s %s failed: %e", src, dst, ret);
}
