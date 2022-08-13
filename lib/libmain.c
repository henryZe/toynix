// Called from entry.S to get us going.
// entry.S already took care of defining envs, pages, uvpd, and uvpt.

#include <lib.h>

/* const volatile struct Env *thisenv; */

void
libmain(int argc, char **argv)
{
	// set thisenv to point at our Env structure in envs[].
	/* thisenv = &envs[ENVX(sys_getenvid())]; */

	/* set default page fault handler */
	set_pgfault_handler(pgfault);

	// call user main routine
	umain(argc, argv);

	// exit gracefully
	exit();
}
