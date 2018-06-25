#include <lib.h>

void
umain(int argc, char **argv)
{
	int i, ret;

	// Spin for a bit to let the console quiet
	for (i = 0; i < 10; ++i)
		sys_yield();

	close(0);

	ret = opencons();
	if (ret < 0)
		panic("opencons: %e", ret);

	if (ret != 0)
		panic("first opencons used fd %d", ret);

	ret = dup(0, 1);
	if (ret < 0)
		panic("dup: %e", ret);

	for (;;) {
		char *buf;

		buf = readline("Type a line: ");
		if (buf)
			fprintf(1, "%s\n", buf);
		else
			fprintf(1, "(end of file received)\n");
	}
}
