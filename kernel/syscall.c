// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <types.h>
#include <error.h>
#include <assert.h>
#include <syscall.h>
#include <debug.h>
#include <ns.h>
#include <string.h>
#include <kernel/env.h>
#include <kernel/pmap.h>
#include <kernel/console.h>
#include <kernel/sched.h>
#include <kernel/env.h>
#include <kernel/time.h>
#include <kernel/e1000.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static int
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.
	user_mem_assert(curenv, s, len, 0);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);

	return 0;
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int ret;
	struct Env *env;

	if ((ret = envid2env(envid, &env, 1)) < 0)
		return ret;

	env_destroy(env);
	return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
	sched_yield();
}

/*
 * This system call creates a new environment with an almost blank slate:
 * nothing is mapped in the user portion of its address space, and it is not runnable.
 * The new environment will have the same register state
 * as the parent environment at the time of the sys_exofork call.
 * In the parent, sys_exofork will return the envid_t of the newly created environment
 * (or a negative error code if the environment allocation failed).
 * In the child, however, it will return 0.
 *
 * Returns envid of new environment, or < 0 on error.  Errors are:
 *     -E_NO_FREE_ENV if no free environment is available.
 *     -E_NO_MEM on memory exhaustion.
 */
static int
sys_exofork(void)
{
	struct Env *env;
	int ret = env_alloc(&env, curenv->env_id);

	if (ret < 0)
		return ret;

	env->env_tf = curenv->env_tf;
	env->env_status = ENV_NOT_RUNNABLE;
	env->env_tf.tf_regs.reg_eax = 0;	/* return 0 back to child */
	strcpy(env->currentpath, curenv->currentpath);
	env->binaryname[0] = '\0';

	return env->env_id;
}

/*
 * Sets the status of a specified environment to
 * ENV_RUNNABLE or ENV_NOT_RUNNABLE.
 * This system call is typically used to mark a new environment ready to run,
 * once its address space and register state has been fully initialized.
 *
 *  Returns 0 on success, < 0 on error.  Errors are:
 *  -E_BAD_ENV if environment envid doesn't currently exist,
 *      or the caller doesn't have permission to change envid.
 *  -E_INVAL if status is not a valid status for an environment.
 */
static int
sys_env_set_status(envid_t envid, int status)
{
	struct Env *env;

	if (status != ENV_NOT_RUNNABLE && status != ENV_RUNNABLE)
		return -E_INVAL;

	if (envid2env(envid, &env, 1) < 0)
		return -E_BAD_ENV;

	env->env_status = status;
	return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3), interrupts enabled, and IOPL of 0.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
	int ret;
	struct Env *env;

	ret = envid2env(envid, &env, 1);
	if (ret < 0)
		return ret;

	user_mem_assert(curenv, tf, sizeof(struct Trapframe), PTE_W);

	/* only set eip & esp */
	env->env_tf.tf_eip = tf->tf_eip;
	env->env_tf.tf_esp = tf->tf_esp;

	env->env_tf.tf_eflags |= FL_IF;		/* interrupts enabled */
	env->env_tf.tf_eflags &= ~FL_IOPL_MASK;	/* IOPL of 0 */

	return 0;
}

/*
 * Allocate a page of memory and map it at 'va' with permission
 * 'perm' in the address space of 'envid'.
 * The page's contents are set to 0.
 * If a page is already mapped at 'va', that page is unmapped as a
 * side effect.
 *
 * perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
 *		but no other bits may be set.
 *
 * Return 0 on success, < 0 on error.	Errors are:
 *	-E_BAD_ENV if environment envid doesn't currently exist,
 *			or the caller doesn't have permission to change envid.
 *	-E_INVAL if va >= UTOP, or va is not page-aligned.
 *	-E_NO_MEM if there's no memory to allocate the new page,
 *			or to allocate any necessary page tables.
 */
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	int ret;
	struct Env *env;
	struct PageInfo *page;

	if (envid2env(envid, &env, 1) < 0)
		return -E_BAD_ENV;

	if (((uint32_t)va % PGSIZE) || ((uint32_t)va >= UTOP))
		return -E_INVAL;

	page = page_alloc(ALLOC_ZERO);
	if (!page)
		return -E_NO_MEM;

	ret = page_insert(env->env_pgdir, page, va, perm | PTE_U);
	if (ret < 0) {
		page_free(page);
		return ret;
	}

	return 0;
}

/*
 * Map the page of memory at 'srcva' in srcenvid's address space
 * at 'dstva' in dstenvid's address space with permission 'perm'.
 * Perm has the same restrictions as in sys_page_alloc, except
 * that it also must not grant write access to a read-only
 * page.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *     -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
 *             or the caller doesn't have permission to change one of them.
 *     -E_INVAL if srcva >= UTOP or srcva is not page-aligned,
 *             or dstva >= UTOP or dstva is not page-aligned.
 *     -E_INVAL is srcva is not mapped in srcenvid's address space.
 *     -E_INVAL if perm is inappropriate (see sys_page_alloc).
 *     -E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
 *             address space.
 *     -E_NO_MEM if there's no memory to allocate any necessary page tables.
 */
static int
sys_page_map(envid_t srcenv_id, void *src,
			envid_t dtsenv_id, void *dst, int perm)
{
	struct Env *src_env, *dst_env;
	struct PageInfo *page;
	pte_t *pte;
	int ret;

	if (((uint32_t)src % PGSIZE) || ((uint32_t)src >= UTOP))
		return -E_INVAL;

	if (((uint32_t)dst % PGSIZE) || ((uint32_t)dst >= UTOP))
		return -E_INVAL;

	if ((envid2env(srcenv_id, &src_env, 1) < 0) ||
		(envid2env(dtsenv_id, &dst_env, 1) < 0))
		return -E_BAD_ENV;

	if (!src) {
		page = pages;	// zero page, read-only
		if (perm & PTE_W)
			return -E_INVAL;
	} else {
		page = page_lookup(src_env->env_pgdir, src, &pte);
		if (!page)
			return -E_INVAL;

		/* src va is read-only but attempt set perm PTE_W */
		if ((~(*pte) & PTE_W) && (perm & PTE_W))
			return -E_INVAL;
	}

	ret = page_insert(dst_env->env_pgdir, page, dst, perm | PTE_U);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * Unmap the page of memory at 'va' in the address space of 'envid'.
 * If no page is mapped, the function silently succeeds.
 *
 * Return 0 on success, < 0 on error.  Errors are:
 *     -E_BAD_ENV if environment envid doesn't currently exist,
 *             or the caller doesn't have permission to change envid.
 *     -E_INVAL if va >= UTOP, or va is not page-aligned.
 */
static int
sys_page_unmap(envid_t envid, void *va)
{
	int ret;
	struct Env *env;
	struct PageInfo *page;

	if (envid2env(envid, &env, 1) < 0)
		return -E_BAD_ENV;

	if (((uint32_t)va % PGSIZE) || ((uint32_t)va >= UTOP))
		return -E_INVAL;

	page_remove(env->env_pgdir, va);
	return 0;
}

/*
 * Set the page fault upcall for 'envid' by modifying the corresponding struct
 * Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
 * kernel will push a fault record onto the exception stack, then branch to
 * 'func'.
 *
 * Returns 0 on success, < 0 on error.  Errors are:
 *     -E_BAD_ENV if environment envid doesn't currently exist,
 *             or the caller doesn't have permission to change envid.
 *     -E_INVAL if upcall func is invalid.
 */
static int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	struct Env *env;

	if (envid2env(envid, &env, 1) < 0)
		return -E_BAD_ENV;

	if (!upcall)
		return -E_INVAL;

	env->env_pgfault_upcall = upcall;
	return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist.
//		(No need to check permissions.)
//	-E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//		or another environment managed to send first.
//	-E_INVAL if srcva < UTOP but srcva is not page-aligned.
//	-E_INVAL if srcva < UTOP and perm is inappropriate
//		(see sys_page_alloc).
//	-E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//		address space.
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in the
//		current environment's address space.
//	-E_NO_MEM if there's not enough memory to map srcva in envid's
//		address space.
static int
sys_ipc_try_send(envid_t envid, int value,
		void *srcva, int perm)
{
	struct Env *env;
	int ret;
	struct PageInfo *page;
	pte_t *pte;

	if (((uint32_t)srcva % PGSIZE) || ((uint32_t)srcva >= UTOP))
		return -E_INVAL;

	ret = envid2env(envid, &env, 0);
	if (ret < 0)
		return ret;

	if (!env->env_ipc_recving)
		return -E_IPC_NOT_RECV;

	if (env->env_ipc_dstva && srcva) {
		/* va2page */
		page = page_lookup(curenv->env_pgdir, srcva, &pte);
		if (!page)
			return -E_INVAL;

		/* check permission */
		if ((~(*pte) & PTE_W) && (perm & PTE_W))
			return -E_INVAL;

		/* page map */
		ret = page_insert(env->env_pgdir, page, env->env_ipc_dstva, perm | PTE_U);
		if (ret < 0)
			return ret;

		env->env_ipc_perm = perm | PTE_U | PTE_P;
	} else {
		env->env_ipc_perm = 0;
	}

	env->env_ipc_value = value;
	env->env_ipc_from = curenv->env_id;
	env->env_ipc_recving = false;

	env->env_status = ENV_RUNNABLE;

	return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//	-E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
	if (((uint32_t)dstva % PGSIZE) || ((uint32_t)dstva >= UTOP))
		return -E_INVAL;

	curenv->env_ipc_recving = true;
	curenv->env_ipc_dstva = dstva;

	curenv->env_tf.tf_regs.reg_eax = 0;	/* return 0 from receiver */
	curenv->env_status = ENV_NOT_RUNNABLE;

	/* not return */
	sched_yield();
}

// Return the current time.
static int
sys_time_msec(void)
{
	return time_msec();
}

static int
sys_debug_info(int option, char *buf, size_t size)
{
	int i, ret = 0;
	struct PageInfo *p = page_free_list;
	const char *env_status[] = {
		"free",
		"dying",
		"waiting",
		"running",
		"pending",
	};

	switch (option) {
	case CPU_INFO:
		ret = snprintf(buf, size, "CPU num: %d\n", ncpu);
		break;

	case MEM_INFO:
		for (i = 0; p != NULL; i++)
			p = p->pp_link;

		ret = snprintf(buf, size, "Total Pages Num: %d\n"
				"Free Pages Num: %d\n"
				"Used Pages Num: %d\n",
				npages, i, npages - i);
		break;

	case ENV_INFO:
		for (i = 0; i < NENV; i++) {
			if (envs[i].env_status != ENV_FREE) {
				cprintf("Env: %x Name: %16s Status: %8s Run Times: %8d Father: %x",
					envs[i].env_id, envs[i].binaryname,
					env_status[envs[i].env_status], envs[i].env_runs,
					envs[i].env_parent_id);
				if (envs[i].env_parent_id)
					cprintf(" %16s", envs[ENVX(envs[i].env_parent_id)].binaryname);
				cprintf("\n");
			}
		}
		break;

	case VMA_INFO:
		break;

	default:
		return -E_INVAL;
	}

	return ret;
}

static int
sys_tx_pkt(const uint8_t *content, uint32_t length)
{
	int ret;
	const uint8_t *addr;
	uint32_t len;
	uint8_t flag;

	user_mem_assert(curenv, content, length, PTE_U);

	/* Align the packet length to jp_len
	 * and set EOP flag at the last descripter.
	 */
	while (length) {
		addr = content;
		len = MIN(length, MAX_JIF_LEN);

		content += len;
		length -= len;
		if (!length)
			flag = E1000_TXD_CMD_EOP;
		else
			flag = 0;

		while (1) {
			if (!e1000_put_tx_desc(addr, len, flag))
				break;

			sys_yield();
		}
	}

	return 0;
}

static int
sys_rx_pkt(uint8_t *content, uint32_t length)
{
	user_mem_assert(curenv, content, length, PTE_U | PTE_W);
	return e1000_get_rx_desc(content, length);
}

static int
sys_chdir(const char *path)
{
	user_mem_assert(curenv, path, strlen(path) + 1, PTE_U);
	strcpy(curenv->currentpath, path);
	return 0;
}

static int
sys_add_vma(envid_t envid, uintptr_t va, size_t memsz, int perm)
{
	int ret;
	struct Env *e;

	ret = envid2env(envid, &e, 1);
	if (ret)
		return ret;

	return env_add_vma(e, va, memsz, PTE_U | perm);
}

static int
sys_copy_vma(envid_t src_env, envid_t dst_env)
{
	int i, ret;
	struct Env *src_e, *dst_e;

	ret = envid2env(src_env, &src_e, 1);
	if (ret)
		return ret;

	ret = envid2env(dst_env, &dst_e, 1);
	if (ret)
		return ret;

	for (i = 0; i < src_e->vma_valid; i++)
		dst_e->vma[i] = src_e->vma[i];
	dst_e->vma_valid = src_e->vma_valid;

	return 0;
}

static int
sys_env_name(envid_t envid, const char *name)
{
	int ret;
	struct Env *env;

	ret = envid2env(envid, &env, 1);
	if (ret)
		return ret;

	user_mem_assert(curenv, name, strlen(name) + 1, PTE_U);
	strcpy(env->binaryname, name);
	return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	switch (syscallno) {
	case SYS_cputs:
		return sys_cputs((const char *)a1, a2);

	case SYS_cgetc:
		return sys_cgetc();

	case SYS_getenvid:
		return sys_getenvid();

	case SYS_env_destroy:
		return sys_env_destroy(a1);

	case SYS_yield:
		sys_yield();
		return 0;

	case SYS_exofork:
		return sys_exofork();

	case SYS_env_set_status:
		return sys_env_set_status(a1, a2);

	case SYS_env_set_trapframe:
		return sys_env_set_trapframe(a1, (void *)a2);

	case SYS_page_alloc:
		return sys_page_alloc(a1, (void *)a2, a3);

	case SYS_page_map:
		return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);

	case SYS_page_unmap:
		return sys_page_unmap(a1, (void *)a2);

	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void *)a2);

	case SYS_ipc_try_send:
		return sys_ipc_try_send(a1, a2, (void *)a3, a4);

	case SYS_ipc_recv:
		return sys_ipc_recv((void *)a1);

	case SYS_time_msec:
		return sys_time_msec();

	case SYS_debug_info:
		return sys_debug_info(a1, (void *)a2, a3);

	case SYS_tx_pkt:
		return sys_tx_pkt((const uint8_t *)a1, a2);

	case SYS_rx_pkt:
		return sys_rx_pkt((uint8_t *)a1, a2);

	case SYS_chdir:
		return sys_chdir((const char *)a1);

	case SYS_add_vma:
		return sys_add_vma(a1, a2, a3, a4);

	case SYS_copy_vma:
		return sys_copy_vma(a1, a2);

	case SYS_env_name:
		return sys_env_name(a1, (const char *)a2);

	default:
		return -E_INVAL;
	}
}
