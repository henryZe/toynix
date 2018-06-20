#include <lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd *)0xCCCCC000)

static int
xopen(const char *path, int mode)
{
	extern union Fsipc fsipcbuf;
	envid_t fs_env;

	strcpy(fsipcbuf.open.req_path, path);
	fsipcbuf.open.req_omode = mode;

	fs_env = ipc_find_env(ENV_TYPE_FS);
	ipc_send(fs_env, FSREQ_OPEN, &fsipcbuf, PTE_W);

	/*
	 * Kernel will map FVA, and pageref++ = 2;
	 * Need to unmap manually.
	 */
	return ipc_recv(NULL, FVA, NULL);
}

void
umain(int argc, char **argv)
{
	int ret, f, i;
	struct Fd *fd;
	struct Fd fdcopy;
	struct Stat st;
	char buf[512];

	/* we open files manually first, to avoid the FD layer */
	ret = xopen("/not-found", O_RDONLY);
	if ((ret < 0) && (ret != -E_NOT_FOUND))
		panic("serve_open /not-found: %e", ret);
	else if (ret >= 0)
		panic("serve_open /not-found succeeded!");

	ret = xopen("/newmotd", O_RDONLY);
	if (ret < 0)
		panic("serve_open /newmotd: %e", ret);

	if (FVA->fd_dev_id != 'f' || FVA->fd_offset != 0 || FVA->fd_omode != O_RDONLY)
		panic("serve_open did not fill struct Fd correctly\n");

	cprintf("serve_open is good\n");

	ret = devfile.dev_stat(FVA, &st);
	if (ret < 0)
		panic("file_stat: %e", ret);

	if (strlen(msg) != st.st_size)
		panic("file_stat returned size %d wanted %d\n", st.st_size, strlen(msg));

	cprintf("file_stat is good\n");

	memset(buf, 0, sizeof(buf));
	ret = devfile.dev_read(FVA, buf, sizeof(buf));
	if (ret < 0)
		panic("file_read: %e", ret);
	if (strcmp(buf, msg))
		panic("file_read returned wrong data\n%s\n%s\n", buf, msg);

	cprintf("file_read is good\n");

	ret = devfile.dev_close(FVA);
	if (ret < 0)
		panic("file_close: %e", ret);

	cprintf("file_close is good\n");

	// We're about to unmap the FD, but still need a way to get
	// the stale filenum to serve_read, so we make a local copy.
	// The file server won't think it's stale until we unmap the
	// FD page.
	fdcopy = *FVA;
	sys_page_unmap(0, FVA);

	ret = devfile.dev_read(&fdcopy, buf, sizeof(buf));
	if (ret != -E_INVAL)
		panic("serve_read does not handle stale fileids correctly: %e", ret);

	cprintf("stale fileid is good\n");

	// Try writing
	ret = xopen("/new-file", O_RDWR | O_CREAT);
	if (ret < 0)
		panic("serve_open /new-file: %e", ret);

	ret = devfile.dev_write(FVA, msg, strlen(msg));
	if (ret != strlen(msg))
		panic("file_write: %e", ret);

	cprintf("file_write is good\n");

	FVA->fd_offset = 0;
	memset(buf, 0, sizeof(buf));

	ret = devfile.dev_read(FVA, buf, sizeof(buf));
	if (ret < 0)
		panic("file_read after file_write: %e", ret);

	if (ret != strlen(msg))
		panic("file_read after file_write returned wrong length: %d", ret);

	if (strcmp(buf, msg) != 0)
		panic("file_read after file_write returned wrong data");

	cprintf("file_read after file_write is good\n");

	// Now we'll try out open
	ret = open("/not-found", O_RDONLY);
	if (ret < 0 && ret != -E_NOT_FOUND)
		panic("open /not-found: %e", ret);
	else if (ret >= 0)
		panic("open /not-found succeeded!");

	ret = open("/newmotd", O_RDONLY);
	if (ret < 0)
		panic("open /newmotd: %e", ret);

	fd = (struct Fd *)(0xD0000000 + ret * PGSIZE);
	if (fd->fd_dev_id != 'f' ||
		fd->fd_offset != 0 ||
		fd->fd_omode != O_RDONLY)
		panic("open did not fill struct Fd correctly\n");

	cprintf("open is good\n");

	// Try files with indirect blocks
	f = open("/big", O_WRONLY | O_CREAT);
	if (f < 0)
		panic("creat /big: %e", f);

	memset(buf, 0, sizeof(buf));
	for (i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
		*(int *)buf = i;

		ret = write(f, buf, sizeof(buf));
		if (ret < 0)
			panic("write /big@%d: %e", i, ret);
	}

	close(f);

	f = open("/big", O_RDONLY);
	if (f < 0)
		panic("open /big: %e", f);

	for (i = 0; i < (NDIRECT * 3) * BLKSIZE; i += sizeof(buf)) {
		*(int *)buf = i;

		ret = readn(f, buf, sizeof(buf));
		if (ret < 0)
			panic("read /big@%d: %e", i, ret);

		if (ret != sizeof(buf))
			panic("read /big from %d returned %d < %d bytes",
			      i, ret, sizeof(buf));

		if (*(int *)buf != i)
			panic("read /big from %d returned bad data %d",
			      i, *(int *)buf);
	}

	close(f);

	cprintf("large file is good\n");
}
