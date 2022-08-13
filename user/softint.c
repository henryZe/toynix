#include <lib.h>

void
umain(int argc, char **argv)
{
	asm volatile("int $14");	// page fault
}
