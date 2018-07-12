#include <lib.h>
#include <x86.h>

void
sleep(int sec)
{
	unsigned now = sys_time_msec();
	unsigned end = now + sec * 1000;	/* 1s = 1000ms */

	if ((int)now < 0 && (int)now > -MAXERROR)
		panic("sys_time_msec: %e", (int)now);
	if (now > end)
		panic("sleep: wrap");

	while (sys_time_msec() < end)
		sys_yield();
}

void
umain(int argc, char **argv)
{
	int i;

	// Wait for the console to calm down
	for (i = 0; i < 50; i++)
		sys_yield();

	cprintf("starting count down: ");
	for (i = 5; i >= 0; i--) {
		cprintf("%d ", i);
		sleep(1);
	}
	cprintf("\n");
	breakpoint();
}
