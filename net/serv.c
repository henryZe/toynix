/*
 * Network server main loop -
 * serves IPC requests from other environments.
 */

#include <net/ns.h>
#include <arch/thread.h>

static envid_t timer_envid;
static envid_t input_envid;
static envid_t output_envid;




#if 0
void
serve_init(uint32_t ipaddr, uint32_t netmask, uint32_t gateway)
{
	int ret;

	panic("no implemented");

	lwip_core_lock();

	uint32_t done = 0;
	tcpip_init(&tcpip_init_done, &done);
	lwip_core_unlock();



}
#endif
static void
tmain(uint32_t arg)
{
	//serve_init(inet_addr(IP), inet_addr(MASK), inet_addr(DEFAULT));
	//serve();
}

void
umain(int argc, char **argv)
{
	envid_t ns_envid = sys_getenvid();

	binaryname = "ns";

	// fork off the timer thread which will send us periodic messages
	timer_envid = fork();
	if (timer_envid < 0)
		panic("error forking");
	else if (timer_envid == 0) {
		/* child */
		timer(ns_envid, TIMER_INTERVAL);
		return;
	}

	// fork off the input thread which will poll the NIC driver for input packets
	input_envid = fork();
	if (input_envid < 0)
		panic("error forking");
	else if (input_envid == 0) {
		input(ns_envid);
		return;
	}

	// fork off the output thread that will send the packets to the NIC driver
	output_envid = fork();
	if (output_envid < 0)
		panic("error forking");
	else if (output_envid == 0) {
		output(ns_envid);
		return;
	}

	// lwIP requires a user threading library;
	// start the library and jump into a thread to continue initialization.
	thread_init();
	thread_create(NULL, "main", tmain, 0);
	thread_yield();
	// never coming here!
}