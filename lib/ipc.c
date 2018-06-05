// User-level IPC library routines

#include <lib.h>

// Receive a value via IPC and return it.
// If 'pg' is nonnull, then any page sent by the sender will be mapped at
//	that address.
// If 'from_env_store' is nonnull, then store the IPC sender's envid in
//	*from_env_store.
// If 'perm_store' is nonnull, then store the IPC sender's page permission
//	in *perm_store (this is nonzero iff a page was successfully
//	transferred to 'pg').
// If the system call fails, then store 0 in *fromenv and *perm (if
//	they're nonnull) and return the error.
// Otherwise, return the value sent by the sender
int
ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
{
	int ret;

	ret = sys_ipc_recv(pg);

	if (from_env_store)
		*from_env_store = ret < 0 ? 0 : thisenv->env_ipc_from;

	if (perm_store)
		*perm_store = ret < 0 ? 0 : thisenv->env_ipc_perm;

	if (ret < 0)
		return ret;

	return thisenv->env_ipc_value;
}

// Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
// This function keeps trying until it succeeds.
// It should panic() on any error other than -E_IPC_NOT_RECV.
void
ipc_send(envid_t to_env, int val, void *pg, int perm)
{
	int ret;

	while ((ret = sys_ipc_try_send(to_env, val, pg, perm)) < 0) {
		if (ret != -E_IPC_NOT_RECV) {
			if (ret == -E_BAD_ENV)
				panic("%s: %e %x", __func__, ret, to_env);
			else
				panic("%s: %e", __func__, ret);
		}

		sys_yield();
	}
}

// Find the first environment of the given type.  We'll use this to
// find special environments.
// Returns 0 if no such environment exists.
envid_t
ipc_find_env(enum EnvType type)
{
	int i;

	for (i = 0; i < NENV; i++)
		if (envs[i].env_type == type)
			return envs[i].env_id;

	return 0;
}
