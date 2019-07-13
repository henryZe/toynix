#include <lib.h>
#include <x86.h>

void
umain(int argc, char **argv)
{
	int i;

	// Wait for the console to calm down
	for (i = 0; i < 50; i++)
		sys_yield();

	cprintf("starting count down: ");
	for (i = 5; i >= 0; i--) {
		cprintf("%d ", i);
		sleep(1);
	}
	cprintf("\n");
	breakpoint();
}
