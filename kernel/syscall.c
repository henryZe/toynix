// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <types.h>
#include <error.h>
#include <assert.h>
#include <syscall.h>
#include <kernel/env.h>
#include <kernel/pmap.h>
#include <kernel/console.h>
#include <kernel/sched.h>
#include <kernel/env.h>

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

	if (env == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, env->env_id);

	env_destroy(env);
	return 0;
}

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

	return env->env_id;
}

/*
 * Sets the status of a specified environment to ENV_RUNNABLE or ENV_NOT_RUNNABLE.
 * This system call is typically used to mark a new environment ready to run,
 * once its address space and register state has been fully initialized.
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

/*
 * Allocates a page of physical memory
 * and maps it at a given virtual address
 * in a given environment's address space.
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

	ret = page_insert(env->env_pgdir, page, va, perm);
	if (ret < 0) {
		page_free(page);
		return ret;
	}

	return 0;
}

/*
 * Copy a page mapping (not the contents of a page!)
 * from one environment's address space to another,
 * leaving a memory sharing arrangement in place
 * so that the new and the old mappings both refer to
 * the same page of physical memory.
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

	page = page_lookup(src_env->env_pgdir, src, &pte);
	if (!page)
		return -E_INVAL;

	if ((~(*pte) & PTE_W) && (perm & PTE_W))
		return -E_INVAL;

	ret = page_insert(dst_env->env_pgdir, page, dst, perm);
	if (ret < 0)
		return ret;

	return 0;
}

/*
 * Unmap a page mapped at a given virtual address in a given environment.
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

	case SYS_page_alloc:
		return sys_page_alloc(a1, (void *)a2, a3);

	case SYS_page_map:
		return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);

	case SYS_page_unmap:
		return sys_page_unmap(a1, (void *)a2);

	case SYS_env_set_pgfault_upcall:
		return sys_env_set_pgfault_upcall(a1, (void *)a2);

	default:
		return -E_INVAL;
	}
}
