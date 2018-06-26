#include <lib.h>

void
umain(int argc, char **argv)
{
	int fd, n, ret;
	char buf[512 + 1];

	binaryname = "icode";

	cprintf("icode startup\n");

	cprintf("icode: open /motd\n");
	fd = open("/motd", O_RDONLY);
	if (fd < 0)
		panic("icode: open /motd: %e", fd);

	cprintf("icode: read /motd\n");
	while (1) {
		n = read(fd, buf, sizeof(buf) - 1);
		if (n <= 0)
			break;
		sys_cputs(buf, n);	
	}
	
	cprintf("icode: close /motd\n");
	close(fd);

	cprintf("icode: spawn /init\n");
	ret = spawnl("/init", "init", "initarg1", "initarg2", NULL);
	if (ret < 0)
		panic("icode: spawn /init: %e", ret);

	cprintf("icode: exiting\n");
}
