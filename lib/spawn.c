#include <lib.h>

// Spawn a child process from a program image loaded from the file system.
// prog: the pathname of the program to run.
// argv: pointer to null-terminated array of pointers to strings,
// 	 which will be passed to the child as its command-line arguments.
// Returns child envid on success, < 0 on failure.



// Spawn, taking command-line arguments array directly on the stack.
// NOTE: Must have a sentinel of NULL at the end of the args
// (none of the args may be NULL).
int
spawnl(const char *prog, const char *arg0, ...)
{
	// We calculate argc by advancing the args until we hit NULL.
	// The contract of the function guarantees that the last
	// argument will always be NULL, and that none of the other
	// arguments will be NULL.
	int i, argc = 0;
	va_list vl;

	va_start(vl, arg0);
	while (va_arg(vl, void *) != NULL)
		argc++;
	va_end(vl);

	// Now that we have the size of the args, do a second pass
	// and store the values in a VLA(very large array), which has the format of argv
	const char *argv[argc + 2];
	argv[0] = arg0;
	argv[argc + 1] = NULL;

	va_start(vl, arg0);
	for (i = 0; i < argc; i++)
		argv[i + 1] = va_arg(vl, const char *);
	va_end(vl);

	return spawn(prog, argv);
}
