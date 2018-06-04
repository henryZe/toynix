// Ping-pong a counter between two processes.
// Only need to start one of these -- splits into two with fork.

#include <lib.h>

void
umain(int argc, char **argv)
{
	envid_t who;
	int i;

	who = fork();
	if (who) {
		// get the ball rolling 
		cprintf("send 0 from %x to %x\n", sys_getenvid(), who);
		ipc_send(who, 0, NULL, 0);
	}

	while (1) {
		i = ipc_recv(&who, NULL, NULL);
		cprintf("%x got %d from %x\n", sys_getenvid(), i, who);
		if (i == 10)
			return;

		i++;
		ipc_send(who, i, NULL, 0);
		if (i == 10)
			return;
	}
}
