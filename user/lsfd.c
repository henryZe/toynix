#include <lib.h>
#include <args.h>

static void
usage(void)
{
	cprintf("%s: lsfd\n", __func__);
	exit();
}

void
umain(int argc, char **argv)
{
	int ret, i, usefprint = 0;
	struct Stat st;
	struct Argstate args;

	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0) {
		if (i == '1')
			usefprint = 1;
		else
			usage();
	}

	for (i = 0; i < MAXFD; i++) {
		ret = fstat(i, &st);
		if (ret >= 0) {
			if (usefprint) {
				fprintf(1, "fd %d: name %s isdir %d size %d dev %s\n",
					i, st.st_name, st.st_isdir,
					st.st_size, st.st_dev->dev_name);
			} else {
				cprintf("fd %d: name %s isdir %d size %d dev %s\n",
					i, st.st_name, st.st_isdir,
					st.st_size, st.st_dev->dev_name);
			}
		}
	}
}
