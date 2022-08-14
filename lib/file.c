#include <lib.h>

#define debug 0

union Fsipc __aligned(PGSIZE) fsipcbuf;

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
	.dev_close = devfile_flush,
	.dev_stat = devfile_stat,
	.dev_trunc = devfile_trunc,
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

	if (debug)
		cprintf("[%08x] %s %d %08x\n",
			thisenv->env_id, __func__,
			type, *(uint32_t *)&fsipcbuf);

	static_assert(sizeof(fsipcbuf) == PGSIZE);
	ipc_send(fsenv, type, &fsipcbuf, PTE_W);
	return ipc_recv(NULL, dstva, NULL);
}

// Flush the file descriptor.  After this the fileid is invalid.
//
// This function is called by fd_close.  fd_close will take care of
// unmapping the FD page from this environment.  Since the server uses
// the reference counts on the FD pages to detect which files are
// open, unmapping it is enough to free up server-side resources.
// Other than that, we just have to make sure our changes are flushed
// to disk.
static int
devfile_flush(struct Fd *fd)
{
	fsipcbuf.flush.req_fileid = fd->fd_file.id;
	return fsipc(FSREQ_FLUSH, NULL);
}

// Read at most 'n' bytes from 'fd' at the current position into 'buf'.
//
// Returns:
//	The number of bytes successfully read.
//	< 0 on error.
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
	// Make an FSREQ_WRITE request to the file system server.
	// Be careful: fsipcbuf.write.req_buf is only so large, but
	// remember that write is always allowed to write *fewer*
	// bytes than requested.
	int ret;

	fsipcbuf.write.req_fileid = fd->fd_file.id;
	fsipcbuf.write.req_n = MIN(n, sizeof(fsipcbuf.write.req_buf));
	memmove(fsipcbuf.write.req_buf, buf, fsipcbuf.write.req_n);

	ret = fsipc(FSREQ_WRITE, NULL);
	if (ret < 0)
		return ret;

	assert(ret <= n);
	assert(ret <= sizeof(fsipcbuf.write.req_buf));

	return ret;
}

static int
devfile_stat(struct Fd *fd, struct Stat *st)
{
	int ret;

	fsipcbuf.stat.req_fileid = fd->fd_file.id;

	ret = fsipc(FSREQ_STAT, NULL);
	if (ret < 0)
		return ret;

	strcpy(st->st_name, fsipcbuf.statRet.ret_name);
	st->st_size = fsipcbuf.statRet.ret_size;
	st->st_isdir = fsipcbuf.statRet.ret_isdir;

	return 0;
}

// Truncate or extend an open file to 'size' bytes
static int
devfile_trunc(struct Fd *fd, off_t newsize)
{
	fsipcbuf.set_size.req_fileid = fd->fd_file.id;
	fsipcbuf.set_size.req_size = newsize;

	return fsipc(FSREQ_SET_SIZE, NULL);
}

// Open a file (or directory).
//
// Returns:
//	The file descriptor index on success
//	-E_BAD_PATH if the path is too long (>= MAXPATHLEN)
//	< 0 for other errors.
int
open(const char *path, int mode)
{
	// Find an unused file descriptor page using fd_alloc.
	// Then send a file-open request to the file server.
	// Include 'path' and 'omode' in request,
	// and map the returned file descriptor page
	// at the appropriate fd address.
	// FSREQ_OPEN returns 0 on success, < 0 on failure.
	//
	// (fd_alloc does not allocate a page, it just returns an
	// unused fd address.  Do you need to allocate a page?)
	//
	// Return the file descriptor index.
	// If any step after fd_alloc fails, use fd_close to free the
	// file descriptor.

	int ret;
	struct Fd *fd;

	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;

	ret = fd_alloc(&fd);
	if (ret < 0)
		return ret;

	strcpy(fsipcbuf.open.req_path, path);
	fsipcbuf.open.req_omode = mode;

	ret = fsipc(FSREQ_OPEN, fd);
	if (ret < 0) {
		fd_close(fd, 0);
		return ret;
	}

	return fd2num(fd);
}

int
remove(const char *path)
{
	if (strlen(path) >= MAXPATHLEN)
		return -E_BAD_PATH;

	strcpy(fsipcbuf.remove.req_path, path);
	return fsipc(FSREQ_REMOVE, NULL);
}

int
mkdir(const char *path)
{
	int fd;

	fd = open(path, O_MKDIR | O_EXCL);
	if (fd < 0)
		return fd;

	close(fd);
	return 0;
}

int
chdir(const char *path)
{
	int fd, ret;
	struct Stat st;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return fd;

	ret = fstat(fd, &st);
	if (ret < 0)
		goto close;

	if (!st.st_isdir) {
		ret = -E_INVAL;
		goto close;
	}

	ret = sys_chdir(path);

close:
	close(fd);
	return ret;
}

int
rename(const char *oldpath, const char *newpath)
{
	if ((strlen(oldpath) >= MAXPATHLEN) ||
		(strlen(newpath) >= MAXPATHLEN))
		return -E_BAD_PATH;

	strcpy(fsipcbuf.rename.src_path, oldpath);
	strcpy(fsipcbuf.rename.dst_path, newpath);

	return fsipc(FSREQ_RENAME, NULL);
}
