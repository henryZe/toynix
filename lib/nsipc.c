#include <lib.h>
#include <ns.h>

#define debug 0

// Send an IP request to the network server, and wait for a reply.
// The request body should be in nsipcbuf, and parts of the response
// may be written back to nsipcbuf.
// type: request code, passed as the simple integer IPC value.
// Returns 0 if successful, < 0 on failure.
static int
nsipc(unsigned int type)
{
	static envid_t nsenv;

	if (nsenv == 0)
		nsenv = ipc_find_env(ENV_TYPE_NS);

	static_assert(sizeof(nsipcbuf) == PGSIZE);

	if (debug)
		cprintf("[%08x] %s %d\n",
			thisenv->env_id, __func__, type);

	ipc_send(nsenv, type, &nsipcbuf, PTE_W);
	return ipc_recv(NULL, NULL, NULL);	/* recv ret_val only */
}

int
nsipc_socket(int domain, int type, int protocol)
{
	nsipcbuf.socket.req_domain = domain;
	nsipcbuf.socket.req_type = type;
	nsipcbuf.socket.req_protocol = protocol;

	return nsipc(NSREQ_SOCKET);
}

int
nsipc_bind(int s, struct sockaddr *name, socklen_t namelen)
{
	nsipcbuf.bind.req_s = s;
	memmove(&nsipcbuf.bind.req_name, name, namelen);
	nsipcbuf.bind.req_namelen = namelen;

	return nsipc(NSREQ_BIND);
}

int
nsipc_listen(int s, int backlog)
{
	nsipcbuf.listen.req_s = s;
	nsipcbuf.listen.req_backlog = backlog;

	return nsipc(NSREQ_LISTEN);
}

int
nsipc_accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
	int ret;

	nsipcbuf.accept.req_s = s;
	nsipcbuf.accept.req_addrlen = *addrlen;

	ret = nsipc(NSREQ_ACCEPT);
	if (ret >= 0) {
		struct Nsret_accept *ret = &nsipcbuf.acceptRet;

		memmove(addr, &ret->ret_addr, ret->ret_addrlen);
		*addrlen = ret->ret_addrlen;
	}

	return ret;
}

int
nsipc_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
	nsipcbuf.connect.req_s = s;
	memmove(&nsipcbuf.connect.req_name, name, namelen);
	nsipcbuf.connect.req_namelen = namelen;

	return nsipc(NSREQ_CONNECT);
}

int
nsipc_recv(int s, void *mem, int len, unsigned int flags)
{
	int ret;

	nsipcbuf.recv.req_s = s;
	nsipcbuf.recv.req_len = len;
	nsipcbuf.recv.req_flags = flags;

	ret = nsipc(NSREQ_RECV);
	if (ret >= 0) {
		assert(ret < 1600 && ret <= len);
		memmove(mem, nsipcbuf.recvRet.ret_buf, ret);
	}

	return ret;
}

int
nsipc_send(int s, const void *buf, int size, unsigned int flags)
{
	assert(size < 1600);

	nsipcbuf.send.req_s = s;
	memmove(&nsipcbuf.send.req_buf, buf, size);
	nsipcbuf.send.req_size = size;
	nsipcbuf.send.req_flags = flags;

	return nsipc(NSREQ_SEND);
}

int
nsipc_shutdown(int s, int how)
{
	nsipcbuf.shutdown.req_s = s;
	nsipcbuf.shutdown.req_how = how;

	return nsipc(NSREQ_SHUTDOWN);
}

int
nsipc_close(int s)
{
	nsipcbuf.close.req_s = s;

	return nsipc(NSREQ_CLOSE);
}
