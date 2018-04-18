// Simple implementation of cprintf console output for the kernel,
// based on printfmt() and the kernel console's cputchar().

#include <types.h>
#include <error.h>
#include <assert.h>

int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	// Call the function corresponding to the 'syscallno' parameter.
	// Return any appropriate return value.
	// LAB 3: Your code here.

	panic("syscall not implemented");

	switch (syscallno) {
	default:
		return -E_INVAL;
	}
}
