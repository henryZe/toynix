#include <lib.h>
#include <x86.h>

char buf[512], buf2[512];

void
umain(int argc, char **argv)
{
	int fd, ret, n, n2;

	fd = open("/motd", O_RDONLY);
	if (fd < 0)
		panic("open motd: %e", fd);

	seek(fd, 0);

	n = readn(fd, buf, sizeof(buf));
	if (n <= 0)
		panic("readn: %e", n);

	ret = fork();
	if (ret < 0)
		panic("fork: %e", ret);

	if (ret == 0) {
		seek(fd, 0);
		cprintf("going to read in child (might page fault if your sharing is buggy)\n");

		n2 = readn(fd, buf2, sizeof(buf2));
		if (n2 != n)
			panic("read in parent got %d, read in child got %d", n, n2);

		if (memcmp(buf, buf2, n) != 0)
			panic("read in parent got different bytes from read in child");

		cprintf("read in child succeeded\n");
		seek(fd, 0);
		close(fd);
		exit();
	}
	wait(ret);

	n2 = readn(fd, buf2, sizeof(buf2));
	if (n2 != n)
		panic("read in parent got %d, then got %d", n, n2);

	if (memcmp(buf, buf2, n) != 0)
		panic("read in parent got different bytes from last time");

	cprintf("read in parent succeeded\n");
	close(fd);

	breakpoint();
}
