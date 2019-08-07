#include <lib.h>

void
cputchar(int ch)
{
	char c = ch;

	// Unlike standard Unix's putchar,
	// the cputchar function _always_ outputs to the system console.
	sys_cputs(&c, 1);
}

int
getchar(void)
{
	unsigned char c;
	int ret;

	// Toynix does, however, support standard _input_ redirection,
	// allowing the user to redirect script files to the shell and such.
	// getchar() reads a character from file descriptor 0.
	ret = read(0, &c, 1);
	if (ret < 0)
		return ret;

	if (ret < 1)
		return -E_EOF;

	return c;
}

int
iscons(int fdnum)
{
	int ret;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	return fd->fd_dev_id == devcons.dev_id;
}

int
opencons(void)
{
	int ret;
	struct Fd *fd;

	ret = fd_alloc(&fd);
	if (ret < 0)
		return ret;

	ret = sys_page_alloc(0, fd, PTE_W | PTE_SHARE);
	if (ret < 0)
		return ret;

	fd->fd_dev_id = devcons.dev_id;
	fd->fd_omode = O_RDWR;

	return fd2num(fd);
}

static ssize_t
devcons_read(struct Fd *fd, void *vbuf, size_t n)
{
	int c;

	if (n == 0)
		return 0;

	while (1) {
		c = sys_cgetc();
		if (c)
			break;
		sys_yield();
	}

	if (c < 0)
		return c;
	if (c == 0x04)	// ctl-d is eof
		return 0;

	*(char *)vbuf = c;
	return 1;
}

static ssize_t
devcons_write(struct Fd *fd, const void *vbuf, size_t n)
{
	int tot, m;
	char buf[128] = {0};

	// mistake: have to nul-terminate arg to sys_cputs,
	// so we have to copy vbuf into buf in chunks and nul-terminate.
	for (tot = 0; tot < n; tot += m) {
		m = MIN(n - tot, sizeof(buf) - 1);
		memmove(buf, (char *)vbuf + tot, m);
		sys_cputs(buf, m);	/* have to nul-terminate arg to sys_cputs */
	}

	return tot;
}

static int
devcons_close(struct Fd *fd)
{
	return sys_page_unmap(0, fd);
}

static int
devcons_stat(struct Fd *fd, struct Stat *stat)
{
	strcpy(stat->st_name, "<cons>");
	return 0;
}

// "Real" console file descriptor implementation.
// The putchar/getchar functions above will still come here by default,
// but now can be redirected to files, pipes, etc., via the fd layer.
struct Dev devcons = {
	.dev_id = 'c',
	.dev_name = "cons",
	.dev_read = devcons_read,
	.dev_write = devcons_write,
	.dev_close = devcons_close,
	.dev_stat = devcons_stat,
};
