#include <lib.h>

void
usage(void)
{
	cprintf("usage: debug_info cpu/mem/fs\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i, fd, ret;
	char buf[128] = {0};
	uint32_t *tmp = UTEMP;

	if (argc == 1)
		usage();

	for (i = 0; i < MAXDEBUGOPT; i++) {
		ret = strcmp(argv[1], debug_option[i]);
		if (!ret)
			break;
	}

	switch (i) {
	case CPU_INFO:
	case MEM_INFO:
	case ENV_INFO:
		fd = opendebug();
		if (fd < 0)
			return;

		write(fd, argv[1], strlen(argv[1]));
		read(fd, buf, sizeof(buf));

		close(fd);
		break;

	case FS_INFO:
		ret = sys_page_alloc(0, tmp, PTE_W);

		ipc_send(ipc_find_env(ENV_TYPE_FS), FSREQ_INFO, tmp, PTE_W);
		ret = ipc_recv(NULL, NULL, NULL);
		if (ret < 0)
			return;

		snprintf(buf, sizeof(buf),
				"Total Blocks Num: %d\n"
				"Used Blocks Num: %d\n",
				tmp[0], tmp[1]);

		sys_page_unmap(0, tmp);
		break;

	default:
		return;
	}

	printf("%s", buf);
	return;
}