#include <lib.h>
#include <ns.h>

#include <jif/jif.h>

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/sys.h"
#include <lwip/stats.h>

#include <netif/etharp.h>

#define PKTMAP		0x10000000

struct jif {
	struct eth_addr *ethaddr;
	envid_t envid;
};

static void
low_level_init(struct netif *netif)
{
	int r;

	netif->hwaddr_len = 6;
	netif->mtu = 1500;
	netif->flags = NETIF_FLAG_BROADCAST;

	// MAC address is hardcoded to eliminate a system call
	netif->hwaddr[0] = 0x52;
	netif->hwaddr[1] = 0x54;
	netif->hwaddr[2] = 0x00;
	netif->hwaddr[3] = 0x12;
	netif->hwaddr[4] = 0x34;
	netif->hwaddr[5] = 0x56;
}

/*
 * jif_output():
 *
 * This function is called by the TCP/IP stack when an IP packet
 * should be sent. It calls the function called low_level_output() to
 * do the actual transmission of the packet.
 *
 */
static err_t
jif_output(struct netif *netif, struct pbuf *p,
			struct ip_addr *ipaddr)
{
    /* resolve hardware address, then send (or queue) packet */
	return etharp_output(netif, p, ipaddr);
}

/*
 * low_level_output():
 *
 * Should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 */
static err_t
low_level_output(struct netif *netif, struct pbuf *p)
{
	int r = sys_page_alloc(0, (void *)PKTMAP, PTE_U|PTE_W|PTE_P);
    if (r < 0)
		panic("jif: could not allocate page of memory");

	struct jif_pkt *pkt = (struct jif_pkt *)PKTMAP;
    struct jif *jif = netif->state;

    char *txbuf = pkt->jp_data;
    int txsize = 0;
    struct pbuf *q;
    for (q = p; q != NULL; q = q->next) {
		/*
		 * Send the data from the pbuf to the interface, one pbuf at a
		 * time. The size of the data in each pbuf is kept in the ->len
		 * variable.
		 */
		if (txsize + q->len > 2000)
			panic("oversized packet, fragment %d txsize %d\n", q->len, txsize);

		memcpy(&txbuf[txsize], q->payload, q->len);
		txsize += q->len;
	}

    pkt->jp_len = txsize;

    ipc_send(jif->envid, NSREQ_OUTPUT, (void *)pkt, PTE_P|PTE_W|PTE_U);
    sys_page_unmap(0, (void *)pkt);

	return ERR_OK;
}

/*
 * jif_init():
 *
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 */
err_t
jif_init(struct netif *netif)
{
	struct jif *jif;
	envid_t *output_envid;

	jif = mem_malloc(sizeof(struct jif));
	if (jif == NULL) {
		LWIP_DEBUGF(NETIF_DEBUG, ("jif_init: out of memory\n"));
		return ERR_MEM;
	}

	output_envid = (envid_t *)netif->state;

	netif->state = jif;
	netif->output = jif_output;
	netif->linkoutput = low_level_output;
	memcpy(&netif->name[0], "en", 2);

	jif->ethaddr = (struct eth_addr *)&(netif->hwaddr[0]);
	jif->envid = *output_envid;

	low_level_init(netif);

	etharp_init();

	// qemu user-net is dumb; if the host OS does not send and ARP request
	// first, the qemu will send packets destined for the host using the mac
	// addr 00:00:00:00:00; do a arp request for the user-net
	// NAT(network address translation) at 10.0.2.2
	uint32_t ipaddr = inet_addr("10.0.2.2");
	etharp_query(netif, (struct ip_addr *)&ipaddr, NULL);

	return ERR_OK;
}