#include <lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned int *)0xf0100000 = 0;
}
