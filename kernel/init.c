#include <kernel/string.h>
#include <kernel/console.h>
#include <kernel/stdio.h>

int
mon_backtrace(int argc, char **argv, void *tf)
{
	// Your code here.
	return 0;
}

// Test the stack backtrace function (lab 1 only)
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
	test_backtrace(5);

	while(1);

	// Drop into the kernel monitor.
//	while (1)
//		monitor(NULL);
}
