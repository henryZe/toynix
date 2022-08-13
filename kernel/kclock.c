/* See COPYRIGHT for copyright information. */

/* Support for reading the NVRAM from the real-time clock. */

#include <x86.h>
#include <kernel/kclock.h>

unsigned int
mc146818_read(unsigned int reg)
{
	outb(IO_RTC, reg);
	return inb(IO_RTC + 1);
}

void
mc146818_write(unsigned int reg, unsigned int datum)
{
	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}
