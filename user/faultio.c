// test user-level fault handler -- alloc pages to fix faults

#include <lib.h>
#include <x86.h>

void
umain(int argc, char **argv)
{
	if (read_eflags() & FL_IOPL_3)
		cprintf("eflags wrong\n");

	/*
	 * this outb to select disk 1 should result in a general protection
	 * fault, because user-level code shouldn't be able to use the io space.
	 */
	outb(0x1F6, 0xE0 | (1<<4));

	cprintf("%s: made it here --- bug\n");
}
