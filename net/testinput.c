#include <net/ns.h>
#include <netif/etharp.h>

static envid_t output_envid;
static envid_t input_envid;
static struct jif_pkt *pkt = (struct jif_pkt *)REQVA;

static void
announce(void)
{
	// We need to pre-announce our IP so we don't have to deal
	// with ARP requests.  Ideally, we would use gratuitous ARP
	// for this, but QEMU's ARP implementation is dumb and only
	// listens for very specific ARP requests, such as requests
	// for the gateway IP.

	uint8_t mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
	uint32_t myip = inet_addr(IP);
	uint32_t gwip = inet_addr(DEFAULT);
	int ret;

	ret = sys_page_alloc(0, pkt, PTE_W);
	if (ret < 0)
		panic("sys_page_map: %e", ret);

	struct etharp_hdr *arp = (struct etharp_hdr *)pkt->jp_data;

	pkt->jp_len = sizeof(*arp);

	memset(arp->ethhdr.dest.addr, 0xFF, ETHARP_HWADDR_LEN);
	memcpy(arp->ethhdr.src.addr, mac, ETHARP_HWADDR_LEN);
	arp->ethhdr.type = htons(ETHTYPE_ARP);
	arp->hwtype = htons(1); // Ethernet
	arp->proto = htons(ETHTYPE_IP);
	arp->_hwlen_protolen = htons((ETHARP_HWADDR_LEN << 8) | 4);
	arp->opcode = htons(ARP_REQUEST);
	memcpy(arp->shwaddr.addr, mac, ETHARP_HWADDR_LEN);
	memcpy(arp->sipaddr.addrw, &myip, 4);
	memset(arp->dhwaddr.addr, 0x00, ETHARP_HWADDR_LEN);
	memcpy(arp->dipaddr.addrw, &gwip, 4);

	ipc_send(output_envid, NSREQ_OUTPUT, pkt, PTE_W);
	sys_page_unmap(0, pkt);
}

static void
hexdump(const char *prefix, const void *data, int len)
{
	int i;
	char buf[80];
	char *end = buf + sizeof(buf);
	char *out = NULL;

	for (i = 0; i < len; i++) {
		if (i % 16 == 0)
			out = buf + snprintf(buf, end - buf,
						"%s%04x   ", prefix, i);
		out += snprintf(out, end - out, "%02x", ((uint8_t *)data)[i]);
		if (i % 16 == 15 || i == len - 1)
			cprintf("%.*s\n", out - buf, buf);
		if (i % 2 == 1)
			*(out++) = ' ';
		if (i % 16 == 7)
			*(out++) = ' ';
	}
}

void
umain(int argc, char **argv)
{
	envid_t ns_envid = sys_getenvid();
	int first = 1;

	output_envid = fork();
	if (output_envid < 0) {
		panic("error forking");
	} else if (output_envid == 0) {
		output(ns_envid);
		return;
	}

	input_envid = fork();
	if (input_envid < 0) {
		panic("error forking");
	} else if (input_envid == 0) {
		input(ns_envid);
		return;
	}

	/* Address Resolution Protocol */
	cprintf("Sending ARP announcement...\n");
	announce();

	while (1) {
		envid_t whom;
		int perm;
		int ret = ipc_recv(&whom, pkt, &perm);

		if (ret < 0)
			panic("ipc_recv: %e", ret);
		if (whom != input_envid)
			panic("IPC from unexpected environment %08x", whom);
		if (ret != NSREQ_INPUT)
			panic("Unexpected IPC %d", ret);

		hexdump("input: ", pkt->jp_data, pkt->jp_len);
		cprintf("\n");

		// Only indicate that we're waiting for packets once
		// we've received the ARP reply
		if (first)
			cprintf("Waiting for packets...\n");
		first = 0;
	}
}
