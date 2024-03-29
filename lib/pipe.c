#include <lib.h>

#define debug 0

#define PIPEBUFSIZ 32		// small to provoke races

struct Pipe {
	off_t p_rpos;			// read position
	off_t p_wpos;			// write position
	uint8_t p_buf[PIPEBUFSIZ];	// data buffer
};

int
pipe(int pfd[2])
{
	int ret;
	struct Fd *fd0, *fd1;
	void *va;

	// allocate the file descriptor table entries
	ret = fd_alloc(&fd0);
	if (ret < 0)
		goto err;

	ret = sys_page_alloc(0, fd0, PTE_W | PTE_SHARE);
	if (ret < 0)
		goto err;

	ret = fd_alloc(&fd1);
	if (ret < 0)
		goto err1;

	ret = sys_page_alloc(0, fd1, PTE_W | PTE_SHARE);
	if (ret < 0)
		goto err1;

	// allocate the pipe structure as first data page in both
	va = fd2data(fd0);

	ret = sys_page_alloc(0, va, PTE_W | PTE_SHARE);
	if (ret < 0)
		goto err2;

	ret = sys_page_map(0, va, 0, fd2data(fd1), PTE_W | PTE_SHARE);
	if (ret < 0)
		goto err3;

	// set up fd structures
	fd0->fd_dev_id = devpipe.dev_id;
	fd0->fd_omode = O_RDONLY;

	fd1->fd_dev_id = devpipe.dev_id;
	fd1->fd_omode = O_WRONLY;

	if (debug)
		cprintf("[%08x] %s create %08x\n",
			thisenv->env_id, __func__, uvpt[PGNUM(va)]);

	pfd[0] = fd2num(fd0);
	pfd[1] = fd2num(fd1);

	return 0;

err3:
	sys_page_unmap(0, va);
err2:
	sys_page_unmap(0, fd1);
err1:
	sys_page_unmap(0, fd0);
err:
	return ret;
}

static int
_pipeisclosed(struct Fd *fd, struct Pipe *p)
{
	int n, nn, ret;

	while (1) {
		n = thisenv->env_runs;

		/* if equals, that is writer/reader is closed already (otherwise, pageref(pipe_data) = 2 * pageref(fd)) */
		ret = (pageref(fd) == pageref(p));

		nn = thisenv->env_runs;
		/* make sure in the same timer interrupt */
		if (n == nn)
			return ret;

		if (n != nn && ret)
			cprintf("pipe race avoided\n");
	}
}

int
pipeisclosed(int fdnum)
{
	struct Fd *fd;
	struct Pipe *p;
	int ret;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	p = (struct Pipe *)fd2data(fd);
	return _pipeisclosed(fd, p);
}

static ssize_t
devpipe_read(struct Fd *fd, void *vbuf, size_t n)
{
	uint8_t *buf;
	size_t i;
	struct Pipe *p;

	p = (struct Pipe *)fd2data(fd);
	if (debug)
		cprintf("[%08x] %s %08x %d rpos %d wpos %d\n",
			thisenv->env_id, __func__,
			uvpt[PGNUM(p)], n, p->p_rpos, p->p_wpos);

	buf = vbuf;
	for (i = 0; i < n; i++) {

		// pipe is empty
		while (p->p_rpos == p->p_wpos) {
			// if we got any data, return it
			if (i > 0)
				return i;

			// if all the writers are gone, not EOF
			if (_pipeisclosed(fd, p))
				return 0;

			// yield and see what happens
			if (debug)
				cprintf("%s yield\n", __func__);

			sys_yield();
		}

		// there's a byte.  take it.
		// wait to increment rpos until the byte is taken!
		buf[i] = p->p_buf[p->p_rpos % PIPEBUFSIZ];
		p->p_rpos++;
	}

	return i;
}

static ssize_t
devpipe_write(struct Fd *fd, const void *vbuf, size_t n)
{
	const uint8_t *buf;
	size_t i;
	struct Pipe *p;

	p = (struct Pipe *)fd2data(fd);
	if (debug)
		cprintf("[%08x] %s %08x %d rpos %d wpos %d\n",
			thisenv->env_id, __func__,
			uvpt[PGNUM(p)], n, p->p_rpos, p->p_wpos);

	buf = vbuf;
	for (i = 0; i < n; i++) {

		/* pipe is full */
		while (p->p_wpos >= p->p_rpos + sizeof(p->p_buf)) {

			/*
			 * if all the readers are gone (it's only writers like us now),
			 * note EOF.
			 */
			if (_pipeisclosed(fd, p))
				return 0;

			/* yield and see what happens */
			if (debug)
				cprintf("%s yield\n", __func__);

			sys_yield();
		}

		// there's room for a byte.  store it.
		// wait to increment wpos until the byte is stored!
		p->p_buf[p->p_wpos % PIPEBUFSIZ] = buf[i];
		p->p_wpos++;
	}

	return i;
}

static int
devpipe_close(struct Fd *fd)
{
	(void)sys_page_unmap(0, fd);
	return sys_page_unmap(0, fd2data(fd));
}

static int
devpipe_stat(struct Fd *fd, struct Stat *stat)
{
	struct Pipe *p = (struct Pipe *)fd2data(fd);

	strcpy(stat->st_name, "<pipe>");
	stat->st_size = p->p_wpos - p->p_rpos;
	stat->st_isdir = 0;
	stat->st_dev = &devpipe;

	return 0;
}

struct Dev devpipe = {
	.dev_id = 'p',
	.dev_name = "pipe",
	.dev_read = devpipe_read,
	.dev_write = devpipe_write,
	.dev_close = devpipe_close,
	.dev_stat = devpipe_stat,
};
