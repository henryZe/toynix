// buggy hello world -- unmapped pointer passed to kernel
// kernel should destroy user environment in response

#include <lib.h>

void
umain(int argc, char **argv)
{
	sys_cputs((char*)1, 1);
}