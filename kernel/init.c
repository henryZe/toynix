#include <kernel/string.h>
#include <kernel/console.h>
#include <kernel/stdio.h>

#include <kernel/x86.h>

#define NESTED_TIME_TEST 5
struct Trapframe;

/* bootstacktop: 0xf010a000
* 
* call func ->
* 1. (esp - 4)
* 2. *esp =  next_eip
* 3. eip = func
* push %ebp			# store ebp
* mov  %esp,%ebp	# set ebp as old_esp
* push %edi

* pop %edi
* pop %ebp
* ret ->				# convert call 
* 1. eip = *esp
* 2. (esp + 4)
*
*
* refer to kernblock.asm:
* ebp[f0109ec8] eip[f0100071 : read_ebp] args[f0101910 00010074 f0109f08 0000001a f01009a8]
* ebp[f0109f18] eip[f0100152 : mon_backtrace] args[00000000 00000000 00000000 0000001a f0101958]
* ebp[f0109f38] eip[f0100134 : test_backtrace] args[00000000 00000001 f0109f64 0000001a f0101958]
* ebp[f0109f58] eip[f0100134 : test_backtrace] args[00000001 00000002 f0109f84 0000001a f0101958]
* ebp[f0109f78] eip[f0100134 : test_backtrace] args[00000002 00000003 f0109fa4 0000001a f0101958]
* ebp[f0109f98] eip[f0100134 : test_backtrace] args[00000003 00000004 f0109fc4 0000001a f010198f]
* ebp[f0109fb8] eip[f0100134 : test_backtrace] args[00000004 00000005 f0109fe4 00000010 00000000]
* ebp[f0109fd8] eip[f01001b2 : init] args[00000005 00000000 0000023c 00000000 00000000]
* ebp[f0109ff8] eip[f010003e : spin (entry.S)] args[00000003 00001003 00002003 00003003 00004003]
*
*/
int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	unsigned long args[5];
	unsigned long ebp;
	unsigned long eip;
	int i;

	/* read old stack pointer */
	ebp = read_ebp();

	cprintf("Stack backtrace:\n");
	while (ebp != 0) {
		eip = *((unsigned long *)ebp + 1);

		for (i = 0; i < NESTED_TIME_TEST; i++)
			args[i] = *((unsigned long *)ebp + 2 + i);

		cprintf("ebp[%08x] eip[%08x] [%08x %08x %08x %08x %08x]\n",
			ebp, eip, args[0], args[1], args[2], args[3], args[4]);

		ebp = *((unsigned long *)ebp);
	}

	return 0;
}

// Test the stack backtrace function
void
test_backtrace(int x)
{
	cprintf("entering test_backtrace %d\n", x);
	if (x > 0)
		test_backtrace(x-1);
	else
		mon_backtrace(0, 0, 0);
	cprintf("leaving test_backtrace %d\n", x);
}

void
init(void)
{
	extern char edata[], end[];

	/*
	* Before doing anything else, complete the ELF loading process.
	* Clear the uninitialized global data (BSS) section of our program.
	* This ensures that all static/global variables start out zero.
	*/
	memset(edata, 0, end - edata);

	/*
	* Initialize the console.
	* Can't call printf until after we do this!
	*/
	cons_init();

	cprintf("Enter toynix...\n");

	// Test the stack backtrace function.
	test_backtrace(NESTED_TIME_TEST);

	while(1);

	// Drop into the kernel monitor.
//	while (1)
//		monitor(NULL);
}
