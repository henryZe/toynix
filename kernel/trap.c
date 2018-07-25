#include <x86.h>
#include <mmu.h>
#include <memlayout.h>
#include <assert.h>
#include <kernel/env.h>
#include <kernel/monitor.h>
#include <kernel/syscall.h>
#include <kernel/cpu.h>
#include <kernel/spinlock.h>
#include <kernel/sched.h>
#include <kernel/pmap.h>
#include <kernel/picirq.h>
#include <kernel/console.h>
#include <kernel/time.h>

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/*
 * Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1,
	(uint32_t)idt
};

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	uint8_t id = thiscpu->cpu_id;

	// The example code here sets up the Task State Segment (TSS) and
	// the TSS descriptor for CPU 0. But it is incorrect if we are
	// running on other CPUs because each CPU has its own kernel stack.
	// Fix the code so that it works for all CPUs.
	//
	// ltr sets a 'busy' flag in the TSS selector, so if you
	// accidentally load the same TSS on more than one CPU, you'll
	// get a triple fault.  If you set up an individual CPU's TSS
	// wrong, you may not get a fault until you try to return from
	// user space on that CPU.

	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - id * (KSTKSIZE + KSTKGAP);
	thiscpu->cpu_ts.ts_ss0 = GD_KD;
	thiscpu->cpu_ts.ts_iomb = sizeof(struct Taskstate);

	// Initialize the TSS slot of the gdt.
	gdt[(GD_TSS0 >> 3) + id] = SEG16(STS_T32A, (uint32_t)(&thiscpu->cpu_ts),
					sizeof(struct Taskstate) - 1, 0);
	gdt[(GD_TSS0 >> 3) + id].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(GD_TSS0 + (id << 3));

	// Load the IDT
	lidt(&idt_pd);
}

void traphandler_0(void);
void traphandler_1(void);
void traphandler_2(void);
void traphandler_3(void);
void traphandler_4(void);
void traphandler_5(void);
void traphandler_6(void);
void traphandler_7(void);
void traphandler_8(void);
void traphandler_10(void);
void traphandler_11(void);
void traphandler_12(void);
void traphandler_13(void);
void traphandler_14(void);
void traphandler_16(void);
void traphandler_17(void);
void traphandler_18(void);
void traphandler_19(void);
void traphandler_48(void);
void irqhandler_0(void);
void irqhandler_1(void);
void irqhandler_4(void);
void irqhandler_7(void);
void irqhandler_14(void);
void irqhandler_19(void);

void
trap_init(void)
{
	// trap
	SETGATE(idt[T_DIVIDE], 1, GD_KT, traphandler_0, 0);
	SETGATE(idt[T_DEBUG], 1, GD_KT, traphandler_1, 0);
	SETGATE(idt[T_NMI], 0, GD_KT, traphandler_2, 0);
	SETGATE(idt[T_BRKPT], 1, GD_KT, traphandler_3, 3);
	SETGATE(idt[T_OFLOW], 1, GD_KT, traphandler_4, 0);
	SETGATE(idt[T_BOUND], 1, GD_KT, traphandler_5, 0);
	SETGATE(idt[T_ILLOP], 1, GD_KT, traphandler_6, 0);
	SETGATE(idt[T_DEVICE], 1, GD_KT, traphandler_7, 0);
	SETGATE(idt[T_DBLFLT], 1, GD_KT, traphandler_8, 0);
	SETGATE(idt[T_TSS], 1, GD_KT, traphandler_10, 0);
	SETGATE(idt[T_SEGNP], 1, GD_KT, traphandler_11, 0);
	SETGATE(idt[T_STACK], 1, GD_KT, traphandler_12, 0);
	SETGATE(idt[T_GPFLT], 1, GD_KT, traphandler_13, 0);
	SETGATE(idt[T_PGFLT], 1, GD_KT, traphandler_14, 0);
	SETGATE(idt[T_FPERR], 1, GD_KT, traphandler_16, 0);
	SETGATE(idt[T_ALIGN], 1, GD_KT, traphandler_17, 0);
	SETGATE(idt[T_MCHK], 1, GD_KT, traphandler_18, 0);
	SETGATE(idt[T_SIMDERR], 1, GD_KT, traphandler_19, 0);

	// soft interrupt
	SETGATE(idt[T_SYSCALL], 0, GD_KT, traphandler_48, 3);

	// interrupt
	SETGATE(idt[IRQ_OFFSET + IRQ_TIMER], 0, GD_KT, irqhandler_0, 3);
	SETGATE(idt[IRQ_OFFSET + IRQ_KBD], 0, GD_KT, irqhandler_1, 3);
	SETGATE(idt[IRQ_OFFSET + IRQ_SERIAL], 0, GD_KT, irqhandler_4, 3);
	SETGATE(idt[IRQ_OFFSET + IRQ_SPURIOUS], 0, GD_KT, irqhandler_7, 3);
	SETGATE(idt[IRQ_OFFSET + IRQ_IDE], 0, GD_KT, irqhandler_14, 3);
	SETGATE(idt[IRQ_OFFSET + IRQ_ERROR], 0, GD_KT, irqhandler_19, 3);

	// Per-CPU setup
	trap_init_percpu();
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static const char *trapname(int trapno)
{
	static const char * const exception_names[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < ARRAY_SIZE(exception_names))
		return exception_names[trapno];

	if (trapno == T_SYSCALL)
		return "System call";
	if (trapno >= IRQ_OFFSET && trapno < IRQ_OFFSET + MAX_IRQS)
		return "Hardware Interrupt";

	return "(unknown trap)";
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p from CPU %d\n", tf, cpunum());
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);

	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());

	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K = fault occurred in user/kernel mode
	// W/R = a write/read caused the fault
	// PR = a protection violation caused the fault (NP = page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
				tf->tf_err & 4 ? "user" : "kernel",
				tf->tf_err & 2 ? "write" : "read",
				tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");

	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  eflag 0x%08x\n", tf->tf_eflags);

	// print user stack
	if (tf->tf_cs & 3) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.
	if ((tf->tf_cs & 0x3) != 0x3) {
		panic("kernel fault va %08x ip %08x tf->tf_cs %08x\n",
				fault_va, tf->tf_eip, tf->tf_cs);
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.
	//
	// Call the environment's page fault upcall, if one exists.  Set up a
	// page fault stack frame on the user exception stack (below
	// UXSTACKTOP), then branch to curenv->env_pgfault_upcall.
	//
	// The page fault upcall might cause another page fault, in which case
	// we branch to the page fault upcall recursively, pushing another
	// page fault stack frame on top of the user exception stack.
	//
	// It is convenient for our code which returns from a page fault
	// (lib/pfentry.S) to have one word of scratch space at the top of the
	// trap-time stack; it allows us to more easily restore the eip/esp. In
	// the non-recursive case, we don't have to worry about this because
	// the top of the regular user stack is free.  In the recursive case,
	// this means we have to leave an extra word between the current top of
	// the exception stack and the new stack frame because the exception
	// stack _is_ the trap-time stack.
	//
	// If there's no page fault upcall, the environment didn't allocate a
	// page for its exception stack or can't write to it, or the exception
	// stack overflows, then destroy the environment that caused the fault.
	//
	if (curenv->env_pgfault_upcall) {
		uintptr_t esp;
		struct UTrapframe *utf;

		if (tf->tf_esp >= (UXSTACKTOP - PGSIZE) && tf->tf_esp < UXSTACKTOP)
			/*
			 * trap-time UTrapframe already in exception stack
			 *
			 * In non-recursive case:
			 * Regular Stack & Exception Stack status
			 *
			 * |-------------------|         |-------------------|
			 * | Regular Stack     |         | Exception Stack 1 |
			 * |-------------------|         |-------------------|
			 * | trap-time eip     |
			 * |-------------------|
			 */
			esp = tf->tf_esp - 4 - sizeof(struct UTrapframe);
		else
			/*
			 * first time trap into exception stack
			 *
			 * In recursive case:
			 * Exception Stack status
			 *
			 * |-----------------------|
			 * | Exception Stack n     |
			 * |-----------------------|
			 * | trap-time eip         |
			 * |-----------------------|
			 * | Exception Stack (n+1) |
			 * |-----------------------|
			 */
			esp = UXSTACKTOP - sizeof(struct UTrapframe);

		user_mem_assert(curenv, (void *)esp, sizeof(struct UTrapframe), PTE_W);

		/* push UTrapframe into exception stack */
		utf = (struct UTrapframe *)esp;
		utf->utf_fault_va = fault_va;
		utf->utf_err = tf->tf_err;
		utf->utf_regs = tf->tf_regs;
		utf->utf_eip = tf->tf_eip;
		utf->utf_eflags = tf->tf_eflags;
		utf->utf_esp = tf->tf_esp;

		/* restore env to _pgfault_upcall and set stack as Exception Stack */
		tf->tf_esp = esp;
		tf->tf_eip = (uintptr_t)curenv->env_pgfault_upcall;

		env_run(curenv);
	}

	/* Or destroy the environment that caused the fault. */
	cprintf("[%08x] user fault va %08x ip %08x\n",
			curenv->env_id, fault_va, tf->tf_eip);

	print_trapframe(tf);
	env_destroy(curenv);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	int32_t ret;

	// Handle processor exceptions.
	switch (tf->tf_trapno) {
	case T_PGFLT:
		page_fault_handler(tf);
		break;

	case T_BRKPT:
	case T_DEBUG:
		monitor(tf);
		break;

	case T_SYSCALL:
		ret = syscall(
				tf->tf_regs.reg_eax,
				tf->tf_regs.reg_edx,
				tf->tf_regs.reg_ecx,
				tf->tf_regs.reg_ebx,
				tf->tf_regs.reg_edi,
				tf->tf_regs.reg_esi);
		tf->tf_regs.reg_eax = ret;
		break;

	case IRQ_OFFSET + IRQ_TIMER:
		// Add time tick increment to clock interrupts.
		// Be careful! In multiprocessors, clock interrupts are
		// triggered on every CPU.
		if (thiscpu == &cpus[0])
			time_tick();

		// Handle clock interrupts. Don't forget to acknowledge the
		// interrupt using lapic_eoi() before calling the scheduler.
		lapic_eoi();
		sched_yield();
		break;

	case IRQ_OFFSET + IRQ_SPURIOUS:
		// Handle spurious interrupts
		// The hardware sometimes raises these because of noise on the
		// IRQ line or other reasons. We don't care.
		cprintf("Spurious interrupt on irq %d\n", IRQ_SPURIOUS);
		print_trapframe(tf);
		break;

	// Handle keyboard and serial interrupts.
	case IRQ_OFFSET + IRQ_KBD:
		kbd_intr();
		break;

	case IRQ_OFFSET + IRQ_SERIAL:
		serial_intr();
		break;

	default:
		// Unexpected trap: The user process or the kernel has a bug.
		print_trapframe(tf);
		if (tf->tf_cs == GD_KT) {
			/* GD_KT | 0 */
			panic("unhandled trap in kernel");
		} else {
			/* GD_UT | 3 */
			cprintf("unhandled trap in user\n");

			/* exit user_env */
			env_destroy(curenv);
			return;
		}
	}
}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF(10th bit) and some versions
	// of GCC rely on DF being clear
	/* disable interrupt when enter trap */
	asm volatile("cld" ::: "cc");

	// Halt the CPU if some other CPU has called panic()
	extern char *panicstr;
	if (panicstr)
		asm volatile("hlt");

	// Re-acqurie the big kernel lock if we were halted in sched_yield()
	if (xchg(&thiscpu->cpu_status, CPU_STARTED) == CPU_HALTED)
		lock_kernel();

	// Check that interrupts are disabled. If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		// Acquire the big kernel lock before doing any
		// serious kernel work.
		lock_kernel();
		assert(curenv);

		/*
		 * Garbage collect when next time-interrupt comes
		 * if current environment is a zombie.
		 */
		if (curenv->env_status == ENV_DYING) {
			env_free(curenv);
			curenv = NULL;
			sched_yield();
		}

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		/* save trap_frame into curenv */
		curenv->env_tf = *tf;

		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// If we made it to this point, then no other environment was
	// scheduled, so we should return to the current environment
	// if doing so makes sense.
	if (curenv && curenv->env_status == ENV_RUNNING)
		env_run(curenv);
	else
		sched_yield();
}
