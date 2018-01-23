#ifndef KERN_MONITOR_H
#define KERN_MONITOR_H

struct Trapframe;

int test_backtrace(int argc, char **argv, struct Trapframe *tf);

// Activate the kernel monitor,
// optionally providing a trap frame indicating the current state
// (NULL if none).
void monitor(struct Trapframe *tf);

// Functions implementing monitor commands.
int mon_help(int argc, char **argv, struct Trapframe *tf);
int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
int mon_backtrace(int argc, char **argv, struct Trapframe *tf);

#endif	// !KERN_MONITOR_H
