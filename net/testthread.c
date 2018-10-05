#include <arch/thread.h>
#include <lib.h>

void
tmain(uint32_t arg)
{
    cprintf("create thread success\n");
}

void
umain(int argc, char **argv)
{
    cprintf("test thread start\n");
    thread_init();
	thread_create(NULL, "main", tmain, 0);
    thread_yield();
}