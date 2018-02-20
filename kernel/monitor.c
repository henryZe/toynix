// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <stdio.h>
#include <string.h>
#include <memlayout.h>
#include <x86.h>
#include <kernel/monitor.h>
#include <kernel/kdebug.h>
#include <kernel/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line

struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display the trace of function call", mon_backtrace },
	{ "showmapping", "Display the physical page info of specified va region mapping", mon_showmapping },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

/*
*	Please refer in Entry.S:
*	movl		$0x0, %ebp
*	movl		$(bootstacktop), %esp
*
*	call func				# 1. (esp - 4); 2. *esp =  next_eip; 3. eip = func
*	push %ebp			# store ebp
*	mov  %esp,%ebp		# set ebp as old_esp
*	push %edi
*
*	pop %edi
*	pop %ebp
*	ret					# convert call: 1. eip = *esp; 2. (esp + 4)
*
*
*	refer to kernel.asm:
*	ebp[f0109ec8] eip[f0100071 : read_ebp] 		args[f0101910 00010074 f0109f08 0000001a f01009a8]
*	ebp[f0109f18] eip[f0100152 : mon_backtrace]	args[00000000 00000000 00000000 0000001a f0101958]
*	ebp[f0109f38] eip[f0100134 : test_backtrace]	args[00000000 00000001 f0109f64 0000001a f0101958]
*	ebp[f0109f58] eip[f0100134 : test_backtrace]	args[00000001 00000002 f0109f84 0000001a f0101958]
*	ebp[f0109f78] eip[f0100134 : test_backtrace]	args[00000002 00000003 f0109fa4 0000001a f0101958]
*	ebp[f0109f98] eip[f0100134 : test_backtrace]	args[00000003 00000004 f0109fc4 0000001a f010198f]
*	ebp[f0109fb8] eip[f0100134 : test_backtrace]	args[00000004 00000005 f0109fe4 00000010 00000000]
*	ebp[f0109fd8] eip[f01001b2 : init]			args[00000005 00000000 0000023c 00000000 00000000]
*	ebp[f0109ff8] eip[f010003e : spin (entry.S)]	args[00000003 00001003 00002003 00003003 00004003]
*
*/
int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	struct Eipdebuginfo info;
	unsigned long ebp;
	unsigned long eip;
	int i, ret;

	/* read old func stack pointer (also new func base pointer) */
	ebp = read_ebp();

	cprintf("Stack backtrace:\n");
	while (ebp != 0) {
		eip = *((unsigned long *)ebp + 1);

		ret = debuginfo_eip(eip, &info);
		if (ret < 0)
			cprintf("debuginfo not found location of eip\n");

		cprintf("ebp[%08x] eip[%08x] ", ebp, eip);
		for (i = 0; i < 5; i++)
			cprintf("%08x ", *((unsigned long *)ebp + 2 + i));

		cprintf("\n%s: %d, %.*s + 0x%x, %d arg(s)",
			info.eip_file, info.eip_line,
			info.eip_fn_namelen, info.eip_fn_name,
			eip - info.eip_fn_addr,
			info.eip_fn_narg);

		if (info.eip_fn_narg) {
			cprintf(": ");
			for (i = 0; i < info.eip_fn_narg; i++)
				cprintf("%.*s ",
					info.eip_fn_arglen[i], info.eip_fn_arg[i]);
		}

		cprintf("\n\n");

		/* switch to parent function */
		ebp = *((unsigned long *)ebp);
	}

	return 0;
}

// Test the stack backtrace function
int
test_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	argc -= 1;

	cprintf("entering test_backtrace %d\n", argc);
	if (argc > 0)
		test_backtrace(argc, NULL, NULL);
	else
		mon_backtrace(0, NULL, NULL);
	cprintf("leaving test_backtrace %d\n", argc);

	return 0;
}

int
mon_showmapping(int argc, char **argv, struct Trapframe *tf)
{
	char *arg1 = argv[1];
	char *arg2 = argv[2];
	char *arg3 = argv[3];
	char bit_w;
	char bit_u;
	char bit_a;
	char bit_d;
	char bit_s;

	pde_t pde;
	uintptr_t va_begin = strtol(arg1, NULL, 16);
	uintptr_t va_last = strtol(arg2, NULL, 16);
	uintptr_t va_pde_begin;
	uintptr_t va_pde_end;

	if (arg1 == NULL || arg2 == NULL || arg3) {
		cprintf("It needs exactly address region!\n");
		return 0;
	}

	if (va_begin > va_last) {
		cprintf("The first argument should not larger than the second argument!\n");
		return 0;
	}

	cprintf("entry\t\tVA range\t\t\tflag\tPA range\n");
	cprintf("============================================================================================\n");

	while (va_begin <= va_last) {
		pde = kern_pgdir[PDX(va_begin)];
		va_pde_begin = ROUNDDOWN(va_begin, PTSIZE);
		va_pde_end = va_pde_begin + PTSIZE - 1;

		if (pde & PTE_P) {
			bit_w = (pde & PTE_W)? 'W': 'R';
			bit_u = (pde & PTE_U)? 'U': 'S';
			bit_a = (pde & PTE_A)? 'A': '-';
			bit_d = (pde & PTE_D)? 'D': '-';
			bit_s = (pde & PTE_PS)? 'S': '-';

			cprintf("\n-PDE[%03x]\t", PDX(va_begin));
			cprintf("[%08x - %08x]\t", va_pde_begin, va_pde_end);
			cprintf("--%c%c%c--%c%cP\t", bit_s, bit_d, bit_a, bit_u, bit_w);

			if (pde & PTE_PS)
				cprintf("[%08x - %08x]\n", PTE_ADDR(pde), PTE_ADDR(pde) + PTSIZE - 1);
			else
				cprintf("\n");

			if (!(pde & PTE_PS)) {
				pde = PTE_ADDR(pde);
				pte_t *pte = (pte_t *)KADDR(pde);

				uintptr_t va_pte_begin = ROUNDDOWN(va_begin, PGSIZE);
				for (size_t i = PTX(va_begin); (i < (1 << 10)) && (va_pte_begin <= va_last);
						va_pte_begin += PGSIZE, i++) {
					if (pte[i] & PTE_P) {
						bit_w = (pde & PTE_W)? 'W': 'R';
						bit_u = (pde & PTE_U)? 'U': 'S';
						bit_a = (pde & PTE_A)? 'A': '-';
						bit_d = (pde & PTE_D)? 'D': '-';
						bit_s = (pde & PTE_PS)? 'S': '-';

						cprintf("|PTE[%03x]\t", i);
						cprintf("[%08x - %08x]\t", va_pte_begin, va_pte_begin + PGSIZE - 1);
						cprintf("--%c%c%c--%c%cP\t", bit_s, bit_d, bit_a, bit_u, bit_w);
						cprintf("[%08x - %08x]\n", PTE_ADDR(pte[i]), PTE_ADDR(pte[i]) + PGSIZE - 1);
					}
				}
			}
		}

		if (va_pde_end == (~0))
			break;

		va_begin = va_pde_end + 1;
	}

	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0) {
			return commands[i].func(argc, argv, tf);
		}
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the Toynix kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
