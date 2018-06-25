/* See COPYRIGHT for copyright information. */

#ifndef KERN_SYSCALL_H
#define KERN_SYSCALL_H
#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

int32_t syscall(uint32_t syscallno, uint32_t a1, uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5);

#endif /* !KERN_SYSCALL_H */
