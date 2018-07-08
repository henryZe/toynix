#include <lib.h>
#include <ns.h>

#define debug 0

union Nsipc nsipcbuf __attribute__((aligned(PGSIZE)));

// Send an IP request to the network server, and wait for a reply.
// The request body should be in nsipcbuf, and parts of the response
// may be written back to nsipcbuf.
// type: request code, passed as the simple integer IPC value.
// Returns 0 if successful, < 0 on failure.
static int
nsipc(unsigned type)
{
	static envid_t nsenv;

	if (nsenv == 0)
		nsenv = ipc_find_env(ENV_TYPE_NS);

	static_assert(sizeof(nsipcbuf) == PGSIZE);

	if (debug)
		cprintf("[%08x] nsipc %d\n", thisenv->env_id, type);

	ipc_send(nsenv, type, &nsipcbuf, PTE_W);
	return ipc_recv(NULL, NULL, NULL);	/* recv ret_val only */
}