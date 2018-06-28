#include <lib.h>
#include <x86.h>

void
wrong(int rfd, int kfd, int off)
{
	char buf[100];
	int n;

	seek(rfd, off);
	seek(kfd, off);

	cprintf("shell produced incorrect output.\n");

	cprintf("expected:\n===\n");
	while ((n = read(kfd, buf, sizeof buf-1)) > 0)
		sys_cputs(buf, n);

	cprintf("===\ngot:\n===\n");
	while ((n = read(rfd, buf, sizeof buf-1)) > 0)
		sys_cputs(buf, n);

	cprintf("===\n");
	exit();
}

void
umain(int argc, char **argv)
{
	char c1, c2;
	int ret, rfd, wfd, kfd, n1, n2, off, nloff;
	int pfds[2];

	close(0);
	close(1);
	opencons();
	opencons();

	rfd = open("/testshell.sh", O_RDONLY);
	if (rfd < 0)
		panic("open testshell.sh: %e", rfd);

	wfd = pipe(pfds);
	if (wfd < 0)
		panic("pipe: %e", wfd);

	wfd = pfds[1];

	cprintf("running sh -x < testshell.sh | cat\n");

	ret = fork();
	if (ret < 0)
		panic("fork: %e", ret);

	if (ret == 0) {
		dup(rfd, 0);
		dup(wfd, 1);
		close(rfd);
		close(wfd);

		ret = spawnl("/sh", "sh", "-x", 0);
		if (ret < 0)
			panic("spawn: %e", ret);

		close(0);
		close(1);

		wait(ret);
		exit();
	}
	close(rfd);
	close(wfd);

	rfd = pfds[0];

	kfd = open("/testshell.key", O_RDONLY);
	if (kfd < 0)
		panic("open testshell.key for reading: %e", kfd);

	nloff = 0;	/* error line offset */
	off = 0;
	while (1) {
		n1 = read(rfd, &c1, 1);
		n2 = read(kfd, &c2, 1);
		if (n1 < 0)
			panic("reading testshell.out: %e", n1);
		if (n2 < 0)
			panic("reading testshell.key: %e", n2);
		if (n1 == 0 && n2 == 0)
			break;
		if (n1 != 1 || n2 != 1 || c1 != c2)
			wrong(rfd, kfd, nloff);
		if (c1 == '\n')
			nloff = off + 1;

		off++;
	}
	cprintf("shell ran correctly\n");

	breakpoint();
}
