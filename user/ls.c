#include <lib.h>

void
usage(void)
{
	cprintf("usage: ls\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i, fd, ret;
	struct File *dir;
	struct Stat stat;
	struct File file;	// 256 Bytes

	binaryname = "ls";

	if (argc != 1)
		usage();

	fd = open(currentpath, O_DIR | O_RDONLY);
	fstat(fd, &stat);

	if (!stat.st_isdir) {
		printf("%s is not directory.\n", stat.st_name);
		goto exit;
	}

	printf("current directory: %s %d\n",
			stat.st_name, stat.st_size);
	for (i = 0; i < stat.st_size; i += ret) {
		ret = read(fd, (void *)&file, sizeof(struct File));
		if (ret < 0) {
			printf("read %s failed: %e\n", stat.st_name, ret);
			goto exit;
		}

		if (file.f_name[0] == '\0')
			continue;

		printf("%c\t%d\t%s\n", (file.f_type == FTYPE_REG)? 'r': 'd',
							file.f_size, file.f_name);
	}

exit:
	close(fd);
	exit();
}
