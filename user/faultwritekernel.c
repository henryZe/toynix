#include <lib.h>

void
umain(int argc, char **argv)
{
	*(unsigned *)0xf0100000 = 0;
}