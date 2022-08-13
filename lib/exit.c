#include <lib.h>

void
exit(void)
{
	/*
	 * The envid_t == 0 is special, and
	 * stands for the current environment.
	 */
	sys_env_destroy(0);
}
