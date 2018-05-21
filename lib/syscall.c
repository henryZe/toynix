#include <types.h>
#include <trap.h>
#include <assert.h>
#include <syscall.h>
#include <env.h>

static inline int32_t
syscall(int num, int check, uint32_t a1, uint32_t a2,
		uint32_t a3, uint32_t a4, uint32_t a5)
{
	int32_t ret;

	// Generic system call: pass system call number in AX,
	// up to five parameters in DX, CX, BX, DI, SI.
	// Interrupt kernel with T_SYSCALL.
	//
	// The "volatile" tells the assembler not to optimize
	// this instruction away just because we don't use the
	// return value.
	//
	// The last clause tells the assembler that this can
	// potentially change the condition codes and arbitrary
	// memory locations.
	asm volatile("int %1\n"
				: "=a" (ret)
				: "i" (T_SYSCALL),
				"a" (num),
				"d" (a1),
				"c" (a2),
				"b" (a3),
				"D" (a4),
				"S" (a5)
				: "cc", "memory");

	if (check && ret > 0)
		panic("syscall %d returned %d", num, ret);

	return ret;
}

void
sys_cputs(const char *s, size_t len)
{
	syscall(SYS_cputs, 0, (uint32_t)s, len, 0, 0, 0);
}

int
sys_cgetc(void)
{
	return syscall(SYS_cgetc, 0, 0, 0, 0, 0, 0);
}

envid_t
sys_getenvid(void)
{
	return syscall(SYS_getenvid, 0, 0, 0, 0, 0, 0);
}

int
sys_env_destroy(envid_t envid)
{
	return syscall(SYS_env_destroy, 1, envid, 0, 0, 0, 0);
}

void
sys_yield(void)
{
	syscall(SYS_yield, 0, 0, 0, 0, 0, 0);
}

int
sys_env_set_status(envid_t envid, int status)
{
	return syscall(SYS_env_set_status, 1, envid, status, 0, 0, 0);
}

int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	return syscall(SYS_page_alloc, 1, envid, (uint32_t)va, perm | PTE_U, 0, 0);
}

int
sys_page_map(envid_t src_env, void *src_va,
			envid_t dst_env, void *dst_va, int perm)
{
	return syscall(SYS_page_map, 1, src_env, (uint32_t)src_va, dst_env, (uint32_t)dst_va, perm | PTE_U);
}

int
sys_page_unmap(envid_t envid, void *va)
{
	return syscall(SYS_page_unmap, 1, envid, (uint32_t)va, 0, 0, 0);
}

int
sys_env_set_pgfault_upcall(envid_t envid, void *upcall)
{
	return syscall(SYS_env_set_pgfault_upcall, 1, envid, (uint32_t)upcall, 0, 0, 0);
}