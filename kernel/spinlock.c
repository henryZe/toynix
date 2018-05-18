#include <x86.h>
#include <memlayout.h>
#include <assert.h>
#include <string.h>
#include <kernel/spinlock.h>
#include <kernel/cpu.h>
#include <kernel/kdebug.h>

// The big kernel lock
struct spinlock kernel_lock = {
#ifdef DEBUG_SPINLOCK
	.name = "kernel_lock"
#endif
};

#ifdef DEBUG_SPINLOCK
// Record the current call stack in pcs[] by following the %ebp chain.
static void
get_caller_pcs(uint32_t pcs[])
{
	uint32_t *ebp;
	int i;

	ebp = (uint32_t *)read_ebp();
	for (i = 0; i < DEBUG_PCS; i++) {
		if (!ebp || ebp < (uint32_t *)ULIM)
			break;
		pcs[i] = ebp[1];            // saved %eip
		ebp = (uint32_t *)ebp[0];   // saved %ebp
	}

	for (; i < DEBUG_PCS; i++)
		pcs[i] = 0;
}

// Check whether this CPU is holding the lock.
static int
holding(struct spinlock *lock)
{
	return lock->locked && lock->cpu == thiscpu;
}
#endif

void
__spin_initlock(struct spinlock *lk, char *name)
{
	lk->locked = 0;

#ifdef DEBUG_SPINLOCK
	lk->name = name;
	lk->cpu = NULL;
#endif
}

// Acquire the lock.
// Loops (spins) until the lock is acquired.
// Holding a lock for a long time may cause
// other CPUs to waste time spinning to acquire it.
void
spin_lock(struct spinlock *lk)
{
#ifdef DEBUG_SPINLOCK
	if (holding(lk))
		panic("CPU %d cannot acquire %s: already holding", cpunum(), lk->name);
#endif

	// The xchg is atomic.
	// It also serializes, so that reads after acquire are not
	// reordered before it. 
	while (xchg(&lk->locked, 1) != 0)
		asm volatile ("pause");

#ifdef DEBUG_SPINLOCK
	// Record info about lock acquisition for debugging.
	lk->cpu = thiscpu;
	get_caller_pcs(lk->pcs);
#endif
}

// Release the lock.
void
spin_unlock(struct spinlock *lk)
{
#ifdef DEBUG_SPINLOCK
	if (!holding(lk)) {
		int i;
		uint32_t pcs[DEBUG_PCS];

		// Nab the acquiring EIP chain before it gets released
		memmove(pcs, lk->pcs, sizeof(pcs));
		cprintf("CPU %d cannot release %s: held by CPU %d\nAcquired at:",
				cpunum(), lk->name, lk->cpu->cpu_id);
		for (i = 0; i < DEBUG_PCS && pcs[i]; i++) {
			struct Eipdebuginfo info;
			if (debuginfo_eip(pcs[i], &info) >= 0)
				cprintf("  %08x %s:%d: %.*s + %x\n", pcs[i],
						info.eip_file, info.eip_line,
						info.eip_fn_namelen, info.eip_fn_name,
						pcs[i] - info.eip_fn_addr);
			else
				cprintf("  %08x\n", pcs[i]);
		}
		panic("spin_unlock");
	}

	lk->pcs[0] = 0;
	lk->cpu = NULL;
#endif

	// The xchg instruction is atomic (i.e. uses the "lock" prefix) with
	// respect to any other instruction which references the same memory.
	// x86 CPUs will not reorder loads/stores across locked instructions
	// (vol 3, 8.2.2). Because xchg() is implemented using asm volatile,
	// gcc will not reorder C statements across the xchg.
	xchg(&lk->locked, 0);
}
