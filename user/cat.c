#include <lib.h>

char buf[8192];

void
cat(int f, char *s)
{
	long n;
	int ret;

	while (1) {
		n = read(f, buf, sizeof(buf));
		if (n > 0) {
			ret = write(1, buf, n);
			if (ret != n)
				panic("write error copying %s: %e", s, ret);
		} else if (n == 0) {
			break;
		} else {
			panic("error reading %s: %e", s, n);
		}
	}
}

void
umain(int argc, char **argv)
{
	int f, i;

	if (argc == 1)
		cat(0, "<stdin>");
	else {
		for (i = 1; i < argc; i++) {
			f = open(argv[i], O_RDONLY);
			if (f < 0)
				printf("can't open %s: %e\n", argv[i], f);
			else {
				cat(f, argv[i]);
				close(f);
			}
		}
	}
}