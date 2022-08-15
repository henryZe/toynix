#include <kernel/e1000.h>
#include <net/ns.h>

void
output(envid_t ns_envid)
{
	sys_env_name(0, "ns_output");

	int ret;

	while (1) {
		//	- read a packet from the network server
		ret = sys_ipc_recv(&nsipcbuf);
		if (ret)
			continue;

		if (thisenv->env_ipc_from != ns_envid ||
		    thisenv->env_ipc_value != NSREQ_OUTPUT)
			continue;

		//	- send the packet to the device driver
		ret = sys_tx_pkt((const uint8_t *)nsipcbuf.pkt.jp_data, nsipcbuf.pkt.jp_len);
		if (ret)
			panic("Output ENV fail to transmit packets");
	}
}
