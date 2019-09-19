// Main public header file for our user-land support library,
// whose code lives in the lib directory.
// This library is roughly our OS's version of a standard C library,
// and is intended to be linked into all user-mode applications
// (NOT the kernel or boot loader).

#ifndef INC_LIB_H
#define INC_LIB_H

#include <types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <error.h>
#include <assert.h>
#include <env.h>
#include <memlayout.h>
#include <syscall.h>
#include <fd.h>
#include <ns.h>
#include <debug.h>

#define USED(x)		(void)(x)

// main user program
void umain(int argc, char **argv);

// libmain.c or entry.S
extern volatile struct Env envs[NENV];
extern const volatile struct PageInfo pages[];
/* extern const volatile struct Env *thisenv; */
#define thisenv (&envs[ENVX(sys_getenvid())])

// exit.c
void exit(void);

// readline.c
char *readline(const char *buf);

// pgfault.c
void set_pgfault_handler(void (*handler)(struct UTrapframe *utf));

// pfentry.S
void _pgfault_upcall(void);

// fork.c
#define	PTE_SHARE	0x400
void pgfault(struct UTrapframe *utf);
envid_t fork(void);
envid_t sfork(void);

// sbrk.c
void *sbrk(intptr_t increment);

// syscall.c
void sys_cputs(const char *string, size_t len);
int	sys_cgetc(void);
envid_t	sys_getenvid(void);
int	sys_env_destroy(envid_t);
void sys_yield(void);
int	sys_env_set_status(envid_t envid, int status);
int	sys_page_alloc(envid_t envid, void *pg, int perm);
int	sys_page_map(envid_t src_env, void *src_pg,
		        envid_t dst_env, void *dst_pg, int perm);
int	sys_page_unmap(envid_t env, void *pg);
int sys_env_set_pgfault_upcall(envid_t envid, void *upcall);
int sys_ipc_try_send(envid_t to_env, int value, void *pg, int perm);
int sys_ipc_recv(void *rcv_pg);
int sys_env_set_trapframe(envid_t envid, struct Trapframe *tf);
unsigned int sys_time_msec(void);
int sys_debug_info(int option, char *buf, size_t size);
int sys_chdir(const char *path);
int sys_add_vma(envid_t envid, uintptr_t va, size_t memsz, int perm);
int sys_copy_vma(envid_t src_env, envid_t dst_env);
int sys_env_name(envid_t envid, const char *name);

static inline envid_t __attribute__((always_inline))
sys_exofork(void)
{
    envid_t ret;

    asm volatile("int %2"
                : "=a" (ret)
                : "a" (SYS_exofork), "i" (T_SYSCALL));

    return ret;
}

// ipc.c
void ipc_send(envid_t to_env, int value, void *pg, int perm);
int32_t ipc_recv(envid_t *from_env_store, void *pg, int *perm_store);
envid_t ipc_find_env(enum EnvType type);

// pageref.c
int	pageref(void *addr);

/* File open modes */
#define	O_RDONLY	0x0000		/* open for reading only */
#define	O_WRONLY	0x0001		/* open for writing only */
#define	O_RDWR		0x0002		/* open for reading and writing */
#define	O_ACCMODE	0x0003		/* mask for above modes */

#define	O_CREAT		0x0100		/* create if nonexistent */
#define	O_TRUNC		0x0200		/* truncate to zero length */
#define	O_EXCL		0x0400		/* error if file already exists */
#define O_MKDIR		0x0800		/* open directory, not regular file */

// file.c
int	open(const char *path, int mode);
int	ftruncate(int fd, off_t size);
int	remove(const char *path);
int	sync(void);

// fd.c
int	close(int fd);
ssize_t	read(int fd, void *buf, size_t nbytes);
ssize_t	write(int fd, const void *buf, size_t nbytes);
int	seek(int fd, off_t offset);
void close_all(void);
ssize_t	readn(int fd, void *buf, size_t nbytes);
int	dup(int oldfd, int newfd);
int	fstat(int fd, struct Stat *statbuf);
int	stat(const char *path, struct Stat *statbuf);
int mkdir(const char *path);
int chdir(const char *path);
int rename(const char *oldpath, const char *newpath);

// spawn.c
envid_t spawn(const char *program, const char **argv);
envid_t spawnl(const char *program, const char *arg0, ...);

// wait.c
void wait(envid_t env);

// console.c
void cputchar(int c);
int getchar(void);
int iscons(int fd);
int opencons(void);

// pipe.c
int	pipe(int pipefds[2]);
int	pipeisclosed(int pipefd);

// sockets.c
int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int bind(int s, struct sockaddr *name, socklen_t namelen);
int shutdown(int s, int how);
int connect(int s, const struct sockaddr *name, socklen_t namelen);
int listen(int s, int backlog);
int socket(int domain, int type, int protocol);

// nsipc.c
int nsipc_accept(int s, struct sockaddr *addr, socklen_t *addrlen);
int nsipc_bind(int s, struct sockaddr *name, socklen_t namelen);
int nsipc_shutdown(int s, int how);
int nsipc_close(int s);
int nsipc_connect(int s, const struct sockaddr *name, socklen_t namelen);
int nsipc_listen(int s, int backlog);
int nsipc_recv(int s, void *mem, int len, unsigned int flags);
int nsipc_send(int s, const void *buf, int size, unsigned int flags);
int nsipc_socket(int domain, int type, int protocol);

// debug
int opendebug(void);

// network
int sys_tx_pkt(const uint8_t *content, uint32_t length);
int sys_rx_pkt(uint8_t *content, uint32_t length);

// sleep
void sleep(int sec);

#endif	// !INC_LIB_H