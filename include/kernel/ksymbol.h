#ifndef KERN_KSYMBOL_H
#define KERN_KSYMBOL_H

#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

#include <stab.h>

extern const struct Stab __STAB_BEGIN__[];	// Beginning of stabs table
extern const struct Stab __STAB_END__[];	// End of stabs table
extern const char __STABSTR_BEGIN__[];		// Beginning of string table
extern const char __STABSTR_END__[];		// End of string table

extern const char _start[], entry[], stext[], etext[], srodata[], erodata[];
extern const char sdata[], edata[], sbss[], end[];

extern const char mpentry_start[], mpentry_end[];

#endif
