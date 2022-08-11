#include <lib.h>

void
umain(int argc, char **argv)
{
	cprintf("%.07f\n", 123.99999);
	cprintf("%.07f\n", 123.004567);
	cprintf("%.05f\n", 123.004567);
	cprintf("%.01f\n", 123.004567);
	cprintf("%04f\n",  123.004567);
	cprintf("%f %f\n", -3.1415, -1.23456789);

	cprintf("%X\n", 0xabcdef);
	cprintf("%x\n", 0xabcdef);

	cprintf("%8d FFFF\n", 123);
	cprintf("%-8d FFFF\n", 123);
	cprintf("%8s FFFF\n", "123");
	cprintf("%-8s FFFF\n", "123");
}
