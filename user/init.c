#include <lib.h>

struct {
	char msg1[5000];
	char msg2[1000];
} data = {
	"this is initialized data",
	"so is this"
};

/* not initialized */
char bss[6000];

int
sum(const char *s, int n)
{
	int i, tot = 0;

	for (i = 0; i < n; i++)
		tot ^= i * s[i];

	return tot;
}

void
umain(int argc, char **argv)
{
	int i, ret, x, want;
	char args[256] = {0};

	cprintf("init: running\n");

	want = 0xf989e;
	x = sum((char *)&data, sizeof(data));
	if (x != want)
		cprintf("init: data is not initialized: "
				"got sum %08x wanted %08x\n",
				x, want);
	else
		cprintf("init: data seems okay\n");

	x = sum(bss, sizeof(bss));
	if (x)
		cprintf("bss is not initialized: "
				"wanted sum 0 got %08x\n", x);
	else
		cprintf("init: bss seems okay\n");

	// being run directly from kernel, so no file descriptors open yet
	close(0);

	ret = opencons();
	if (ret < 0)
		panic("opencons: %e", ret);
	if (ret != 0)
		panic("first opencons used fd %d", ret);

	ret = dup(0, 1);
	if (ret < 0)
		panic("dup: %e", ret);

	// output in one syscall per line to avoid output interleaving
	strcat(args, "init: args:");
	for (i = 0; i < argc; i++) {
		strcat(args, " '");
		strcat(args, argv[i]);
		strcat(args, "'");
	}
	printf("%s\n", args);

	while (1) {
		printf("init: starting sh\n");

		ret = spawnl("/sh", "sh", NULL);
		if (ret < 0) {
			printf("init: spawn sh: %e\n", ret);
			continue;
		}

		wait(ret);
	}
}
