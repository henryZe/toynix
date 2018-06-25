#include <lib.h>

#define debug 0

#define PIPEBUFSIZ 32		// small to provoke races

struct Pipe {
	off_t p_rpos;				// read position
	off_t p_wpos;				// write position
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
		cprintf("[%08x] pipe create %08x\n", thisenv->env_id, uvpt[PGNUM(va)]);

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

		
	}
}

int pipeisclosed(int fdnum)
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
		cprintf("[%08x] devpipe_read %08x %d rpos %d wpos %d\n",
			thisenv->env_id, uvpt[PGNUM(p)], n, p->p_rpos, p->p_wpos);

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
				cprintf("devpipe_read yield\n");

			sys_yield();
		}

		// there's a byte.  take it.
		// wait to increment rpos until the byte is taken!
		buf[i] = p->p_buf[p->p_rpos % PIPEBUFSIZ];
		p->p_rpos++;
	}

	return i;
}




struct Dev devpipe = {
	.dev_ip = 'p',
	.dev_name = "pipe",
	.dev_read = devpipe_read,
	.dev_write = devpipe_write,
	.dev_close = devpipe_close,
	.dev_stat = devpipe_stat,
};