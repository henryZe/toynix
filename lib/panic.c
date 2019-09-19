#include <types.h>
#include <lib.h>
#include <x86.h>

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception,
 * which causes OS to enter the kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);

	// Print the panic message
	cprintf("[%08x] user panic in %s at %s:%d: ",
			sys_getenvid(), thisenv->binaryname, file, line);
	vcprintf(fmt, ap);
	cprintf("\n");

	// Cause a breakpoint exception
	while (1)
		breakpoint();
}
