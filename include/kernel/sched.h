#ifndef KERN_SCHED_H
#define KERN_SCHED_H
#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

// This function does not return.
void __noreturn sched_yield(void);

#endif	// !KERN_SCHED_H
