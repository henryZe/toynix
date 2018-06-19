#include <lib.h>
#include <fd.h>

#define debug 0

union Fsipc fsipcbuf __attribute__((aligned(PGSIZE)));

static int devfile_flush(struct Fd *fd);
static ssize_t devfile_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfile_write(struct Fd *fd, const void *buf, size_t n);
static int devfile_stat(struct Fd *fd, struct Stat *stat);
static int devfile_trunc(struct Fd *fd, off_t newsize);

struct Dev devfile = {
	.dev_id = 'f',
	.dev_name = "file",
	.dev_read = devfile_read,
	.dev_write = devfile_write,
/*	.dev_close = devfile_flush,
	.dev_stat = devfile_stat,
	.dev_trunc = devfile_trunc,*/
};

// Send an inter-environment request to the file server, and wait for
// a reply.  The request body should be in fsipcbuf, and parts of the
// response may be written back to fsipcbuf.
// type: request code, passed as the simple integer IPC value.
// dstva: virtual address at which to receive reply page, 0 if none.
// Returns result from the file server.
static int
fsipc(unsigned int type, void *dstva)
{
	static envid_t fsenv;
	if (!fsenv)
		fsenv = ipc_find_env(ENV_TYPE_FS);

	static_assert(sizeof(fsipcbuf) == PGSIZE);

	if (debug)
		cprintf("[%08x] fsipc %d %08x\n", thisenv->env_id, type, *(uint32_t *)&fsipcbuf);

	ipc_send(fsenv, type, &fsipcbuf, PTE_W);
	return ipc_recv(NULL, dstva, NULL);
}

// Read at most 'n' bytes from 'fd' at the current position into 'buf'.
//
// Returns:
// 	The number of bytes successfully read.
// 	< 0 on error.
static ssize_t
devfile_read(struct Fd *fd, void *buf, size_t n)
{
	// Make an FSREQ_READ request to the file system server after
	// filling fsipcbuf.read with the request arguments.  The
	// bytes read will be written back to fsipcbuf by the file
	// system server.
	int ret;

	fsipcbuf.read.req_fileid = fd->fd_file.id;
	fsipcbuf.read.req_n = n;

	ret = fsipc(FSREQ_READ, NULL);
	if (ret < 0)
		return ret;

	assert(ret <= n);
	/* sizeof ret_buf is PGSIZE */
	assert(ret <= PGSIZE);

	memmove(buf, fsipcbuf.readRet.ret_buf, ret);
	return ret;
}

// Write at most 'n' bytes from 'buf' to 'fd' at the current seek position.
//
// Returns:
//	 The number of bytes successfully written.
//	 < 0 on error.
static ssize_t
devfile_write(struct Fd *fd, const void *buf, size_t n)
{
	// Make an FSREQ_WRITE request to the file system server.  Be
	// careful: fsipcbuf.write.req_buf is only so large, but
	// remember that write is always allowed to write *fewer*
	// bytes than requested.
	// LAB 5: Your code here
	panic("devfile_write not implemented");
}
