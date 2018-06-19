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





}
