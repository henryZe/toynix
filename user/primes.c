#include <lib.h>

unsigned
primeproc(void)
{
	int i, id, p;
	envid_t envid;

	// fetch a prime from our left neighbor
top:
	/* recv from own parent */
	p = ipc_recv(&envid, 0, 0);
	cprintf("CPU %d: %d ", thisenv->env_cpunum, p);

	// fork a right neighbor to continue the chain
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		/* child */
		goto top;

	// filter out multiples of our prime
	while (1) {
		/* recv from origin parent */
		i = ipc_recv(&envid, 0, 0);
		if (i % p)
			/* send to own child */
			ipc_send(id, i, 0, 0);
	}
}

void
umain(int argc, char **argv)
{
	int i, id;

	// fork the first prime process in the chain
	if ((id = fork()) < 0)
		panic("fork: %e", id);
	if (id == 0)
		/* child */
		primeproc();

	// feed all the integers through
	for (i = 2; ; i++)
		/* parent */
		ipc_send(id, i, 0, 0);
}
