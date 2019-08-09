#include <net/ns.h>

extern union Nsipc nsipcbuf;

void
input(envid_t ns_envid)
{
	binaryname = "ns_input";

	int ret;
	uint32_t length;

	while (1) {
		ret = sys_page_alloc(0, &nsipcbuf, PTE_W);
		if (ret < 0)
			continue;

		// - read a packet from the device driver
		while (1) {
			ret = sys_rx_pkt((uint8_t *)nsipcbuf.pkt.jp_data, MAX_JIF_LEN);
			if (ret < 0)
				sys_yield();
			else
				break;
		}
		nsipcbuf.pkt.jp_len = ret;

		// - send it to the network server
		// Hint: When you IPC a page to the network server, it will be
		// reading from it for a while, so don't immediately receive
		// another packet in to the same physical page.
		while (1) {
			ret = sys_ipc_try_send(ns_envid, NSREQ_INPUT, &nsipcbuf, PTE_W);
			if (ret < 0) {
				if (ret == -E_IPC_NOT_RECV)
					sys_yield();
				else
					panic("input: sys_ipc_try_send failed\n");
			} else {
				/*
				 * Actually we don't need to unmap nsipcbuf,
				 * because it will be remapped in next routine.
				 */
				sys_page_unmap(0, &nsipcbuf);
				break;
			}
		}
	}
}