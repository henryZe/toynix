#include <lib.h>

static int
fd2sockid(int fd)
{
	struct Fd *sfd;
	int ret;

	ret = fd_lookup(fd, &sfd);
	if (ret < 0)
		return ret;

	if (sfd->fd_dev_id != devsock.dev_id)
		return -E_NOT_SUPP;

	return sfd->fd_sock.sockid;
}

static int
alloc_sockfd(int sockid)
{
	struct Fd *sfd;
	int ret;

	ret = fd_alloc(&sfd);
	if (ret < 0)
		goto close_sock;

	ret = sys_page_alloc(0, sfd, PTE_W | PTE_SHARE);
	if (ret < 0)
		goto close_sock;

	sfd->fd_dev_id = devsock.dev_id;
	sfd->fd_omode = O_RDWR;
	sfd->fd_sock.sockid = sockid;

	return fd2num(sfd);

close_sock:
	nsipc_close(sockid);
	return ret;
}

int
socket(int domain, int type, int protocol)
{
	int ret;

	ret = nsipc_socket(domain, type, protocol);
	if (ret < 0)
		return ret;

	return alloc_sockfd(ret);
}

int
bind(int s, struct sockaddr *name, socklen_t namelen)
{
	int ret;

	ret = fd2sockid(s);
	if (ret < 0)
		return ret;

	return nsipc_bind(ret, name, namelen);
}

int
listen(int s, int backlog)
{
	int ret;

	ret = fd2sockid(s);
	if (ret < 0)
		return ret;

	return nsipc_listen(ret, backlog);
}

int
accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int ret;

	ret = fd2sockid(s);
	if (ret < 0)
		return ret;

	ret = nsipc_accept(ret, addr, addrlen);
	if (ret < 0)
		return ret;

	return alloc_sockfd(ret);
}

int
connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	int ret;

	ret = fd2sockid(s);
	if (ret < 0)
		return ret;

	return nsipc_connect(ret, name, namelen);
}

static ssize_t
devsock_read(struct Fd *fd, void *buf, size_t n)
{
	return nsipc_recv(fd->fd_sock.sockid, buf, n, 0);
}

static ssize_t
devsock_write(struct Fd *fd, const void *buf, size_t n)
{
	return nsipc_send(fd->fd_sock.sockid, buf, n, 0);
}

static int
devsock_stat(struct Fd *fd, struct Stat *stat)
{
	strcpy(stat->st_name, "<sock>");
	return 0;
}

int
shutdown(int s, int how)
{
	int ret;

	ret = fd2sockid(s);
	if (ret < 0)
		return ret;

	return nsipc_shutdown(ret, how);
}

static int
devsock_close(struct Fd *fd)
{
	if (pageref(fd) == 1)
		return nsipc_close(fd->fd_sock.sockid);
	else
		return 0;
}

struct Dev devsock = {
	.dev_id = 's',
	.dev_name =	"sock",
	.dev_read =	devsock_read,
	.dev_write = devsock_write,
	.dev_close = devsock_close,
	.dev_stat =	devsock_stat,
};
