/* implement fork from user space */

#include <lib.h>

// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *)utf->utf_fault_va;
	void *va = ROUNDDOWN(addr, PGSIZE);
	uint32_t err = utf->utf_err;
	int ret;

	/*
	 * Check that the faulting access was:
	 * (1) a write;
	 * (2) to a copy-on-write page;
	 *
	 * If not, panic.
	 */
	if (!(err & FEC_WR) || !(uvpt[PGNUM(addr)] & PTE_COW))
		panic("%s addr: %x err: %x pte: %x", __func__,
				addr, err, uvpt[PGNUM(addr)]);

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	ret = sys_page_alloc(0, PFTEMP, PTE_W);
	if (ret < 0)
		panic("sys_page_alloc: %e", ret);

	memcpy(PFTEMP, va, PGSIZE);

	ret = sys_page_map(0, PFTEMP, 0, va, PTE_W);
	if (ret < 0)
		panic("sys_page_map: %e", ret);

	ret = sys_page_unmap(0, PFTEMP);
	if (ret < 0)
		panic("sys_page_unmap: %e", ret);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
static int
duppage(envid_t dst_env, unsigned int pn)
{
	int perm = PGOFF(uvpt[pn]);
	int ret;

	/*
	 * If the page table entry has the PTE_SHARE bit set,
	 * just copy the mapping directly.
	 */
	if (perm & PTE_SHARE) {
		ret = sys_page_map(0, (void *)(pn << PGSHIFT),
				dst_env, (void *)(pn << PGSHIFT), perm);
		if (ret < 0)
			panic("sys_page_map: %e", ret);

		return 0;
	}

	/* copy-on-write */
	if ((perm & PTE_W) || (perm & PTE_COW)) {
		perm &= (~PTE_W);
		perm |= PTE_COW;

		/* map page to child */
		ret = sys_page_map(0, (void *)(pn << PGSHIFT),
				dst_env, (void *)(pn << PGSHIFT), perm);
		if (ret < 0)
			panic("sys_page_map: %e", ret);

		/* remap page self */
		ret = sys_page_map(0, (void *)(pn << PGSHIFT),
				0, (void *)(pn << PGSHIFT), perm);
		if (ret < 0)
			panic("sys_page_map: %e", ret);

		return 0;
	}

	/* read-only */
	ret = sys_page_map(0, (void *)(pn << PGSHIFT),
			dst_env, (void *)(pn << PGSHIFT), perm);
	if (ret < 0)
		panic("sys_page_map: %e", ret);

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
envid_t
fork(void)
{
	int ret, envid;
	uintptr_t va;

	envid = sys_exofork();
	if (!envid) {
		/* child */
		/* thisenv = &envs[ENVX(sys_getenvid())]; */
		return 0;
	}

	/* only dup-page from 0 to USTACKTOP */
	for (va = 0; va < USTACKTOP; va += PGSIZE) {
		if ((uvpd[PDX(va)] & PTE_P) &&
			(uvpt[PGNUM(va)] & PTE_P)) {

			ret = duppage(envid, PGNUM(va));
			if (ret < 0)
				return ret;
		}
	}

	ret = sys_copy_vma(0, envid);
	if (ret < 0)
		panic("%s: %e", __func__, ret);

	/* allocate page for child exception stack */
	ret = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W);
	if (ret < 0)
		panic("%s: %e", __func__, ret);

	/* set upcall by kernel invoking */
	ret = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if (ret < 0)
		panic("%s: %e", __func__, ret);

	ret = sys_env_name(envid, (const char *)thisenv->binaryname);
	if (ret < 0)
		panic("%s: %e", __func__, ret);

	ret = sys_env_set_status(envid, ENV_RUNNABLE);
	if (ret < 0)
		panic("%s: %e", __func__, ret);

	return envid;
}

static int
share_page(envid_t dst_env, unsigned int pn)
{
	int ret, perm = PGOFF(uvpt[pn]);

	ret = sys_page_map(0, (void *)(pn << PGSHIFT),
			dst_env, (void *)(pn << PGSHIFT), perm);
	if (ret < 0)
		panic("sys_page_map: %e", ret);

	return 0;
}

// User-level fork with shared-memory.
envid_t
sfork(void)
{
	int ret, envid;
	uintptr_t va;

	envid = sys_exofork();
	if (!envid)
		/* child */
		return 0;

	/* only dup-page from 0 to (FILEDATA + MAXFD * PGSIZE) */
	for (va = 0; va < (FILEDATA + MAXFD * PGSIZE); va += PGSIZE) {
		if ((uvpd[PDX(va)] & PTE_P) &&
			(uvpt[PGNUM(va)] & PTE_P)) {

			ret = share_page(envid, PGNUM(va));
			if (ret < 0)
				return ret;
		}
	}

	/* only dup-page in User Stack Area */
	for (; va < USTACKTOP; va += PGSIZE) {
		if ((uvpd[PDX(va)] & PTE_P) &&
			(uvpt[PGNUM(va)] & PTE_P)) {

			ret = duppage(envid, PGNUM(va));
			if (ret < 0)
				return ret;
		}
	}

	ret = sys_copy_vma(0, envid);
	if (ret < 0)
		panic("fork: %e", ret);

	/* allocate page for child exception stack */
	ret = sys_page_alloc(envid, (void *)(UXSTACKTOP - PGSIZE), PTE_W);
	if (ret < 0)
		panic("fork: %e", ret);

	/* set upcall by kernel invoking */
	ret = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if (ret < 0)
		panic("fork: %e", ret);

	ret = sys_env_name(envid, (const char *)thisenv->binaryname);
	if (ret < 0)
		panic("fork: %e", ret);

	ret = sys_env_set_status(envid, ENV_RUNNABLE);
	if (ret < 0)
		panic("fork: %e", ret);

	return envid;
}
