#include <kernel/string.h>
#include <kernel/console.h>
#include <kernel/stdio.h>

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

	cprintf("6828 decimal is %o octal!\n", 6828);

	while(1);
#if 0
	// Test the stack backtrace function (lab 1 only)
	test_backtrace(5);

	// Drop into the kernel monitor.
	while (1)
		monitor(NULL);
#endif
}
