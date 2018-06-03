#include <x86.h>
#include <mmu.h>
#include <kernel/env.h>
#include <kernel/monitor.h>
#include <kernel/pmap.h>
#include <kernel/cpu.h>
#include <kernel/spinlock.h>

void sched_halt(void);

// Choose a user environment to run and run it.
void
sched_yield(void)
{
	struct Env *idle;
	int i, j;

	// Implement simple round-robin scheduling.
	//
	// Search through 'envs' for an ENV_RUNNABLE environment in
	// circular fashion starting just after the env this CPU was
	// last running.  Switch to the first such environment found.
	//
	// If no envs are runnable, but the environment previously
	// running on this CPU is still ENV_RUNNING, it's okay to
	// choose that environment.
	//
	// Never choose an environment that's currently running on
	// another CPU (env_status == ENV_RUNNING). If there are
	// no runnable environments, simply drop through to the code
	// below to halt the cpu.
	idle = curenv;
	i = idle ? (ENVX(idle->env_id) + 1) % NENV : 0;

	for (j = 0; j < NENV; j++, i = (i + 1) % NENV) {
		if (envs[i].env_status == ENV_RUNNABLE)
			env_run(envs + i);
	}

	/* If there is no other runnable task, run current task. */
	if (idle && idle->env_status == ENV_RUNNING)
		env_run(idle);

	// sched_halt never returns
	sched_halt();
}

// Halt this CPU when there is nothing to do. Wait until the
// timer interrupt wakes it up. This function never returns.
void
sched_halt(void)
{
	int i;

	// For debugging and testing purposes, if there are no runnable
	// environments in the system, then drop into the kernel monitor.
	for (i = 0; i < NENV; i++) {
		if (envs[i].env_status == ENV_RUNNABLE ||
			envs[i].env_status == ENV_RUNNING ||
			envs[i].env_status == ENV_DYING)
			break;
	}
	if (i == NENV) {
		cprintf("No runnable environments in the system!\n");
		while (1)
			monitor(NULL);
	}

	// Mark that no environment is running on this CPU
	curenv = NULL;
	lcr3(PADDR(kern_pgdir));

	// Mark that this CPU is in the HALT state, so that when
	// timer interupts come in, we know we should re-acquire the
	// big kernel lock.
	xchg(&thiscpu->cpu_status, CPU_HALTED);

	// Release the big kernel lock as if we were "leaving" the kernel
	unlock_kernel();

	// Reset stack pointer, enable interrupts and then halt.
	asm volatile(
		"movl $0, %%ebp\n"	// reset ebp
		"movl %0, %%esp\n"	// push ts_esp0 (stack top) to esp
		"pushl $0\n"		// push 0 as eip
		"pushl $0\n"		// push 0 as ebp
		"sti\n"				// restore interrupt
		"1:\n"
		"hlt\n"				// halt this cpu and wait for interrupt
		"jmp 1b\n"
		: : "a" (thiscpu->cpu_ts.ts_esp0));
}