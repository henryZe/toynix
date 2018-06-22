#ifndef INC_SYSCALL_H
#define INC_SYSCALL_H

/* system call numbers */
enum {
	SYS_cputs = 0,
	SYS_cgetc,
	SYS_getenvid,
	SYS_env_destroy,
	SYS_yield,
	SYS_exofork,
	SYS_env_set_status,
	SYS_page_alloc,
	SYS_page_map,
	SYS_page_unmap,
	SYS_env_set_pgfault_upcall,
	SYS_ipc_try_send,
	SYS_ipc_recv,
	SYS_env_set_trapframe,
	NUM_SYSCALLS
};

#endif /* !INC_SYSCALL_H */
