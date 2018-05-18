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
		return ;

	case SYS_page_alloc:
		return ;

	case SYS_page_map:
		return ;

	case SYS_page_unmap:
		return ;

	default:
		return -E_INVAL;
	}
}
