// evil hello world -- kernel pointer passed to kernel
// kernel should destroy user environment in response

#include <lib.h>

void
umain(int argc, char **argv)
{
	// try to print the kernel entry point as a string!
	sys_cputs((char *)0xf010000c, 100);
}
