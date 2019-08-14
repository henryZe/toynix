#include <kernel/time.h>
#include <assert.h>

static unsigned int ticks;

void
time_init(void)
{
	ticks = 0;
}

// This should be called once per timer interrupt.  A timer interrupt
// fires every 10 ms.
void
time_tick(void)
{
	ticks++;
	if ((ticks << 1) < ticks)
		panic("%s: time overflowed", __func__);
}

unsigned int
time_msec(void)
{
	return ticks * 10;	/* unit: 10 ms */
}
