#include <lib.h>

void
umain(int argc, char **argv)
{
	cprintf("I read %08x from location 0xf0100000!\n", *(unsigned *)0xf0100000);
}