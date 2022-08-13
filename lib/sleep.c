#include <lib.h>

void
sleep(int sec)
{
	unsigned int now = sys_time_msec();
	unsigned int end = now + sec * 1000;	/* 1s = 1000ms */

	if ((int)now < 0 && (int)now > -MAXERROR)
		panic("sys_time_msec: %e", (int)now);
	if (now > end)
		panic("sleep: wrap");

	while (sys_time_msec() < end)
		sys_yield();
}
