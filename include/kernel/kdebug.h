#ifndef KERN_KDEBUG_H
#define KERN_KDEBUG_H

#include <types.h>

#define MAXARGS 8

// Debug information about a particular instruction pointer
struct Eipdebuginfo {
	const char *eip_file;				// Source code filename for EIP
	int eip_line;						// Source code linenumber for EIP

	const char *eip_fn_name;			// Name of function containing EIP
	int eip_fn_namelen;					// Length of function name
	uintptr_t eip_fn_addr;				// Address of start of function
	int eip_fn_narg;					// Number of function arguments
	const char *eip_fn_arg[MAXARGS];	// List of arguments name
	int eip_fn_arglen[MAXARGS];			// Length of arguments name
};

int debuginfo_eip(uintptr_t eip, struct Eipdebuginfo *info);

#endif
