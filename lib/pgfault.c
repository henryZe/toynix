/*
 * User-level page fault handler support.
 * Rather than register the C page fault handler directly with the
 * kernel as the page fault handler, we register the assembly language
 * wrapper in pfentry.S, which in turns calls the registered C
 * function.
 */

#include <lib.h>

// Pointer to currently installed C-language pgfault handler.
void (*_pgfault_handler)(struct UTrapframe *utf) = NULL;

// Set the page fault handler function.
// If there isn't one yet, _pgfault_handler will be 0.
// The first time we register a handler, we need to
// allocate an exception stack (one page of memory with its top
// at UXSTACKTOP), and tell the kernel to call the assembly-language
// _pgfault_upcall routine when a page fault occurs.
void
set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
{
	int ret;

	if (!_pgfault_handler) {
		/* First time through */
		ret = sys_page_alloc(0, (void *)(UXSTACKTOP - PGSIZE), PTE_W);
		if (ret < 0)
			panic("%s: %e", __func__, ret);

		ret = sys_env_set_pgfault_upcall(0, _pgfault_upcall);
		if (ret < 0)
			panic("%s: %e", __func__, ret);
	}

	/* call back by assembly function _pgfault_upcall */
	_pgfault_handler = handler;
}
