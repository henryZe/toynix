#include <lib.h>

#define debug		0

// Return the 'struct Fd*' for file descriptor index i
#define INDEX2FD(i)	((struct Fd *)(FDTABLE + (i) * PGSIZE))
// Return the file data page for file descriptor index i
#define INDEX2DATA(i)	((char *)(FILEDATA + (i) * PGSIZE))

// --------------------------------------------------------------
// File descriptor manipulators
// --------------------------------------------------------------

int
fd2num(struct Fd *fd)
{
	return ((uintptr_t)fd - FDTABLE) / PGSIZE;
}

char *
fd2data(struct Fd *fd)
{
	return INDEX2DATA(fd2num(fd));
}

// Finds the smallest i from 0 to MAXFD-1 that doesn't have
// its fd page mapped.
// Sets *fd_store to the corresponding fd page virtual address.
//
// fd_alloc does NOT actually allocate an fd page.
// It is up to the caller to allocate the page somehow.
// This means that if someone calls fd_alloc twice in a row
// without allocating the first page we return, we'll return the same
// page the second time.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_MAX_FD: no more file descriptors
// On error, *fd_store is set to 0.
int
fd_alloc(struct Fd **fd_store)
{
	int i;
	struct Fd *fd;

	for (i = 0; i < MAXFD; i++) {
		fd = INDEX2FD(i);
		if ((uvpd[PDX(fd)] & PTE_P) == 0 ||
			(uvpt[PGNUM(fd)] & PTE_P) == 0) {
			/* means free */
			*fd_store = fd;
			return 0;
		}
	}

	*fd_store = NULL;
	return -E_MAX_OPEN;
}

// Check that fdnum is in range and mapped.
// If it is, set *fd_store to the fd page virtual address.
//
// Returns 0 on success (the page is in range and mapped), < 0 on error.
// Errors are:
//	-E_INVAL: fdnum was either not in range or not mapped.
int
fd_lookup(int fdnum, struct Fd **fd_store)
{
	struct Fd *fd;

	if (fdnum < 0 || fdnum >= MAXFD) {
		if (debug)
			cprintf("[%08x] bad fd %d\n", thisenv->env_id, fdnum);
		return -E_INVAL;
	}

	fd = INDEX2FD(fdnum);
	if ((uvpd[PDX(fd)] & PTE_P) == 0 || (uvpt[PGNUM(fd)] & PTE_P) == 0) {
		if (debug)
			cprintf("[%08x] closed fd %d\n", thisenv->env_id, fdnum);
		return -E_INVAL;
	}

	*fd_store = fd;
	return 0;
}

// Frees file descriptor 'fd' by closing the corresponding file
// and unmapping the file descriptor page.
// If 'must_exist' is 0, then fd can be a closed or nonexistent file
// descriptor; the function will return 0 and have no other effect.
// If 'must_exist' is 1, then fd_close returns -E_INVAL when passed a
// closed or nonexistent file descriptor.
// Returns 0 on success, < 0 on error.
int
fd_close(struct Fd *fd, bool must_exist)
{
	struct Fd *fd2;
	struct Dev *dev;
	int ret;

	ret = fd_lookup(fd2num(fd), &fd2);
	if ((ret < 0) || (fd != fd2))
		return must_exist ? ret : 0;

	ret = dev_lookup(fd->fd_dev_id, &dev);
	if (ret >= 0) {
		if (dev->dev_close)
			ret = (*dev->dev_close)(fd);
		else
			ret = 0;
	}

	// Make sure fd is unmapped.  Might be a no-op if
	// (*dev->dev_close)(fd) already unmapped it.
	sys_page_unmap(0, fd);

	return ret;
}

// --------------------------------------------------------------
// File functions
// --------------------------------------------------------------

static struct Dev *devtab[] =
{
	&devfile,
	&devpipe,
	&devcons,
	&devdebug,
	&devsock,
	0,
};

int
dev_lookup(int dev_id, struct Dev **dev)
{
	int i;

	for (i = 0; devtab[i]; i++) {
		if (devtab[i]->dev_id == dev_id) {
			*dev = devtab[i];
			return 0;
		}
	}

	cprintf("[%08x] unknown device type %d\n",
			thisenv->env_id, dev_id);
	*dev = NULL;

	return -E_INVAL;
}

int
close(int fdnum)
{
	struct Fd *fd;
	int ret;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	return fd_close(fd, 1);
}

void
close_all(void)
{
	int i;
	for (i = 0; i < MAXFD; i++)
		close(i);
}

ssize_t
write(int fdnum, const void *buf, size_t n)
{
	int ret;
	struct Dev *dev;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	ret = dev_lookup(fd->fd_dev_id, &dev);
	if (ret < 0)
		return ret;

	if ((fd->fd_omode & O_ACCMODE) == O_RDONLY) {
		cprintf("[%08x] write %d -- bad mode\n", thisenv->env_id, fdnum);
		return -E_INVAL;
	}

	if (debug)
		cprintf("write %d %p %d via dev %s\n",
			fdnum, buf, n, dev->dev_name);

	if (!dev->dev_write)
		return -E_NOT_SUPP;

	return (*dev->dev_write)(fd, buf, n);
}

/*
 * size_t: unsigned int
 * ssize_t: signed size_t, int
 */
ssize_t
read(int fdnum, void *buf, size_t n)
{
	int ret;
	struct Dev *dev;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	ret = dev_lookup(fd->fd_dev_id, &dev);
	if (ret < 0)
		return ret;

	if ((fd->fd_omode & O_ACCMODE) == O_WRONLY) {
		cprintf("[%08x] read %d -- bad mode\n",
				thisenv->env_id, fdnum);
		return -E_INVAL;
	}

	if (!dev->dev_read)
		return -E_NOT_SUPP;

	return (*dev->dev_read)(fd, buf, n);
}

ssize_t
readn(int fdnum, void *buf, size_t n)
{
	int m, tot;

	for (tot = 0; tot < n; tot += m) {
		m = read(fdnum, (char *)buf + tot, n - tot);
		if (m < 0)
			return m;
		if (m == 0)
			break;
	}

	return tot;
}

int
seek(int fdnum, off_t offset)
{
	int ret;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	fd->fd_offset = offset;
	return 0;
}

// Make file descriptor 'newfdnum' a duplicate of file descriptor 'oldfdnum'.
// For instance, writing onto either file descriptor will affect the
// file and the file offset of the other.
// Closes any previously open file descriptor at 'newfdnum'.
// This is implemented using virtual memory tricks (of course!).
int
dup(int oldfdnum, int newfdnum)
{
	int ret;
	char *ova, *nva;
	pte_t pte;
	struct Fd *oldfd, *newfd;

	ret = fd_lookup(oldfdnum, &oldfd);
	if (ret < 0)
		return ret;

	close(newfdnum);

	newfd = INDEX2FD(newfdnum);
	ova = fd2data(oldfd);
	nva = fd2data(newfd);

	if ((uvpd[PDX(ova)] & PTE_P) && (uvpt[PGNUM(ova)] & PTE_P)) {
		ret = sys_page_map(0, ova, 0, nva, uvpt[PGNUM(ova)] & PTE_SYSCALL);
		if (ret < 0)
			return ret;
	}

	ret = sys_page_map(0, oldfd, 0, newfd, uvpt[PGNUM(oldfd)] & PTE_SYSCALL);
	if (ret < 0)
		goto err;

	return newfdnum;

err:
	sys_page_unmap(0, nva);
	return ret;
}

int
fstat(int fdnum, struct Stat *stat)
{
	int ret;
	struct Dev *dev;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	ret = dev_lookup(fd->fd_dev_id, &dev);
	if (ret < 0)
		return ret;

	if (!dev->dev_stat)
		return -E_NOT_SUPP;

	stat->st_name[0] = 0;
	stat->st_size = 0;
	stat->st_isdir = 0;
	stat->st_dev = dev;

	return (*dev->dev_stat)(fd, stat);
}

int
stat(const char *path, struct Stat *stat)
{
	int fd, ret;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return fd;

	ret = fstat(fd, stat);

	close(fd);
	return ret;
}

int
ftruncate(int fdnum, off_t size)
{
	int ret;
	struct Dev *dev;
	struct Fd *fd;

	ret = fd_lookup(fdnum, &fd);
	if (ret < 0)
		return ret;

	ret = dev_lookup(fd->fd_dev_id, &dev);
	if (ret < 0)
		return ret;

	if (!dev->dev_trunc)
		return -E_NOT_SUPP;

	return (*dev->dev_trunc)(fd, size);
}