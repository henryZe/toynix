#include <lib.h>

void
umain(int argc, char **argv)
{
	cprintf("I read %08x from location 0!\n", *(unsigned int *)0);
}
