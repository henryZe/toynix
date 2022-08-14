#include <lib.h>

struct regs {
	struct PushRegs regs;
	uintptr_t eip;
	uint32_t eflags;
	uintptr_t esp;
};

#define SAVE_REGS(base) \
	"\tmovl %%edi, 0x00+"base"\n" \
	"\tmovl %%esi, 0x04+"base"\n" \
	"\tmovl %%ebp, 0x08+"base"\n" \
	"\tmovl %%ebx, 0x10+"base"\n" \
	"\tmovl %%edx, 0x14+"base"\n" \
	"\tmovl %%ecx, 0x18+"base"\n" \
	"\tmovl %%eax, 0x1c+"base"\n" \
	"\tmovl %%esp, 0x28+"base"\n"

#define LOAD_REGS(base) \
	"\tmovl 0x00+"base", %%edi\n" \
	"\tmovl 0x04+"base", %%esi\n" \
	"\tmovl 0x08+"base", %%ebp\n" \
	"\tmovl 0x10+"base", %%ebx\n" \
	"\tmovl 0x14+"base", %%edx\n" \
	"\tmovl 0x18+"base", %%ecx\n" \
	"\tmovl 0x1c+"base", %%eax\n" \
	"\tmovl 0x28+"base", %%esp\n"

static struct regs before, during, after;

static void
check_regs(struct regs *a, const char *an, struct regs *b,
		const char *bn, const char *testname)
{
	int mismatch = 0;

	cprintf("%-6s %-8s %-8s\n", "", an, bn);

#define CHECK(name, field)						\
	do {								\
		cprintf("%-6s %08x %08x ", #name, a->field, b->field);	\
		if (a->field == b->field)				\
			cprintf("OK\n");				\
		else {							\
			cprintf("MISMATCH\n");				\
			mismatch = 1;					\
		}							\
	} while (0)

	CHECK(edi, regs.reg_edi);
	CHECK(esi, regs.reg_esi);
	CHECK(ebp, regs.reg_ebp);
	CHECK(ebx, regs.reg_ebx);
	CHECK(edx, regs.reg_edx);
	CHECK(ecx, regs.reg_ecx);
	CHECK(eax, regs.reg_eax);
	CHECK(eip, eip);
	CHECK(eflags, eflags);
	CHECK(esp, esp);

#undef CHECK

	cprintf("Registers %s ", testname);
	if (!mismatch)
		cprintf("OK\n");
	else
		cprintf("MISMATCH\n");
}

static void
fault_regs_pgfault(struct UTrapframe *utf)
{
	int ret;

	if (utf->utf_fault_va != (uint32_t)UTEMP)
		panic("pgfault expected at UTEMP, got 0x%08x (eip %08x)",
				utf->utf_fault_va, utf->utf_eip);

	// Check registers in UTrapframe
	during.regs = utf->utf_regs;
	during.eip = utf->utf_eip;
	during.eflags = utf->utf_eflags & ~FL_RF;
	during.esp = utf->utf_esp;
	check_regs(&before, "before", &during, "during", "in UTrapframe");

	// Map UTEMP so the write succeeds
	ret = sys_page_alloc(0, UTEMP, PTE_W);
	if (ret < 0)
		panic("sys_page_alloc: %e", ret);
}

void
umain(int argc, char **argv)
{
	set_pgfault_handler(fault_regs_pgfault);

	asm volatile(
		"\tpushl %%eax\n"

		// Light up eflags to catch more errors
		"\tpushfl\n"
		"\tpopl %%eax\n"
		"\torl $0x8d5, %%eax\n"
		"\tpushl %%eax\n"
		"\tpopfl\n"

		// Save before registers
		/* before->eflags */
		"\tmov %%eax, 0x24 + %0\n"
		/* before->eip */
		"\tleal 0f, %%eax\n"
		"\tmovl %%eax, 0x20 + %0\n"

		/* restore eax */
		"\tpopl %%eax\n"

		// save others
		SAVE_REGS("%0")

		// fault at UTEMP
		"\t0: movl $42, 0x400000\n"

		// Save after registers (except eip and eflags)
		SAVE_REGS("%1")
		// Restore registers (except eip and eflags).  This
		// way, the test will run even if EIP is the *only*
		// thing restored correctly.
		LOAD_REGS("%0")

		// Save after eflags (now that stack is back); note
		// that we were very careful not to modify eflags in
		// since we saved it
		"\tpushl %%eax\n"
		"\tpushfl\n"
		"\tpopl %%eax\n"
		/* after->eflags */
		"\tmov %%eax, 0x24+%1\n"
		"\tpopl %%eax\n"
		: : "m" (before), "m" (after) : "memory", "cc");

	// Check UTEMP to roughly determine that EIP was restored
	// correctly (of course, we probably wouldn't get this far if
	// it weren't)
	if (*(int *)UTEMP != 42)
		cprintf("EIP after page-fault MISMATCH\n");
	after.eip = before.eip;

	check_regs(&before, "before", &after, "after", "after page-fault");
}
