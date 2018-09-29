#include <lib.h>

int
opendebug(void)
{
	int ret;
	struct Fd *fd;

	ret = fd_alloc(&fd);
	if (ret < 0)
		return ret;

	ret = sys_page_alloc(0, fd, PTE_W | PTE_SHARE);
	if (ret < 0)
		return ret;

	fd->fd_dev_id = devdebug.dev_id;
	fd->fd_omode = O_RDWR;
	fd->fd_debg.option = 0;

	return fd2num(fd);
}

static ssize_t
devdebug_read(struct Fd *fd, void *vbuf, size_t n)
{
	return sys_debug_info(fd->fd_debg.option, vbuf, n);
}

static ssize_t
devdebug_write(struct Fd *fd, const void *vbuf, size_t n)
{
	int i;

	for (i = 0; i < MAXDEBUGOPT; i++) {
		if (!strncmp(vbuf, debug_option[i], n)) {
			fd->fd_debg.option = i;
			break;
		}
	}

	if (i == MAXDEBUGOPT)
		return -EINVAL;

	printf("select option: %s\n", debug_option[i]);
	return n;
}

static int
devdebug_close(struct Fd *fd)
{
	return sys_page_unmap(0, fd);
}

static int
devdebug_stat(struct Fd *fd, struct Stat *stat)
{
	strcpy(stat->st_name, "<debug>");
	return 0;
}

struct Dev devdebug = {
	.dev_id = 'd',
	.dev_name = "debug",
	.dev_read = devdebug_read,
	.dev_write = devdebug_write,
	.dev_close = devdebug_close,
	.dev_stat = devdebug_stat,
};
