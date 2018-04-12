#ifndef INC_SYSCALL_H
#define INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_getenvid,
	SYS_env_destroy,
	NUM_SYSCALLS
};

#endif /* !INC_SYSCALL_H */
