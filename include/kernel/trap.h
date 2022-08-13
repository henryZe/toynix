/* See COPYRIGHT for copyright information. */

#ifndef KERN_TRAP_H
#define KERN_TRAP_H
#ifndef TOYNIX_KERNEL
# error "This is a Toynix kernel header; user programs should not #include it"
#endif

#include <trap.h>
#include <mmu.h>

extern const char *panicstr;

void trap_init(void);
void trap_init_percpu(void);
void print_trapframe(struct Trapframe *tf);
void trap(struct Trapframe *tf);

#endif /* KERN_TRAP_H */
