// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <lib.h>

void
umain(int argc, char **argv)
{
	envid_t who;

	who = fork();
	if (who) {
		// get the ball rolling 
		cprintf("send 0 from %08x to %08x\n", sys_getenvid(), who);
		ipc_send(who, 0, NULL, 0);
	}

	while (1) {
		uint32_t i = ipc_recv(&who, NULL, NULL);
		cprintf("%08x got %d from %08x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;

		i++;
		ipc_send(who, i, NULL, 0);
		if (i == 10)
			return;
	}
}
