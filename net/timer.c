#include <net/ns.h>

void
timer(envid_t envid, uint32_t initial_to)
{
	int ret;
	uint32_t stop = sys_time_msec() + initial_to;

	sys_env_name(0, "ns_timer");

	while (1) {
		while (1) {
			ret = sys_time_msec();
			if (ret < stop && ret >= 0)
				sys_yield();
			else if (ret < 0)
				panic("sys_time_msec: %e", ret);
			else
				break;
		}

		ipc_send(envid, NSREQ_TIMER, NULL, 0);

		while (1) {
			uint32_t to;
			envid_t whom;

			to = ipc_recv(&whom, NULL, NULL);
			if (whom != envid) {
				cprintf("NS TIMER: timer thread got IPC message from env %x not NS\n", whom);
				continue;
			}

			stop = sys_time_msec() + to;
			break;
		}
	}
}
