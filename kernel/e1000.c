#include <kernel/e1000.h>
#include <kernel/pmap.h>
#include <stdio.h>
#include <error.h>
#include <string.h>

#define NTXDESCS	64
#define NRXDESCS	128

#define E1000_REG_ADDR(base, offset) ((uintptr_t)(base) + (offset))

volatile uint32_t *e1000;
volatile uint32_t *e1000_tdt;
volatile uint32_t *e1000_rdt;

static struct tx_desc tx_desc_table[NTXDESCS];
static struct rx_desc rx_desc_table[NRXDESCS];

int
pci_e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);

	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	/* initialize tx_desc_table */
	int i;
	struct tx_desc tx_dec = {
		.addr = 0,
		.length = 0,
		.cso = 0,
		.cmd = 0,
		.status = E1000_TXD_STAT_DD,
		.css = 0,
		.special = 0,
	};

	for (i = 0; i < NTXDESCS; i++)
		tx_desc_table[i] = tx_dec;

	uintptr_t tdbal = E1000_REG_ADDR(e1000, E1000_TDBAL);
	*(uint32_t *)tdbal = PADDR(tx_desc_table);
	uintptr_t tdbah = E1000_REG_ADDR(e1000, E1000_TDBAH);
	*(uint32_t *)tdbah = 0;

	uintptr_t tdlen = E1000_REG_ADDR(e1000, E1000_TDLEN);
	*(uint32_t *)tdlen = sizeof(tx_desc_table);

	uintptr_t tdh = E1000_REG_ADDR(e1000, E1000_TDH);
	*(uint32_t *)tdh = 0;
	uintptr_t tdt = E1000_REG_ADDR(e1000, E1000_TDT);
	*(uint32_t *)tdt = 0;
	e1000_tdt = (uint32_t *)tdt;

	uint32_t tflag = 0;
	uintptr_t tctl = E1000_REG_ADDR(e1000, E1000_TCTL);

	tflag |= E1000_TCTL_EN;
	tflag |= E1000_TCTL_PSP;
	tflag |= E1000_TCTL_COLD(0x40);
	*(uint32_t *)tctl = tflag;

	uint32_t tipg = E1000_REG_ADDR(e1000, E1000_TIPG);
	*(uint32_t *)tipg = 10;
	/* IPGR1 and IPGR2 are not needed in full duplex */

#if 0 /* For e1000_put_tx_desc test */
	int ret;
	struct tx_desc td = {0};
	char *content = "packet~capture";

	td.addr = PADDR(content);
	td.length = strlen(content);
	td.cmd = E1000_TXD_CMD_EOP;

	for (i = 0; i < (NTXDESCS + 1); i++) {
		ret = e1000_put_tx_desc(&td);
		if (ret < 0)
			cprintf("ret = %d\n", ret);
	}
#endif

	uintptr_t ral0 = E1000_REG_ADDR(e1000, E1000_RAL);
	uintptr_t rah0 = E1000_REG_ADDR(e1000, E1000_RAH);
	*(uint32_t *)ral0 = 0x12005452;	/* QEMU default MAC 52:54:00:12:34:56 */
	*(uint32_t *)rah0 = 0x5634 | E1000_RAH_AV;

	uintptr_t rdbal = E1000_REG_ADDR(e1000, E1000_RDBAL);
	*(uint32_t *)rdbal = PADDR(rx_desc_table);
	uintptr_t rdbah = E1000_REG_ADDR(e1000, E1000_RDBAH);
	*(uint32_t *)rdbah = 0;

	for (i = 0; i < NRXDESCS; i++)
		rx_desc_table[i].addr = page2pa(page_alloc(0)) + sizeof(int);	/* struct jif_pkt -> jp_len */

	uintptr_t rdlen = E1000_REG_ADDR(e1000, E1000_RDLEN);
	*(uint32_t *)rdlen = sizeof(rx_desc_table);

	uintptr_t rdh = E1000_REG_ADDR(e1000, E1000_RDH);
	*(uint32_t *)rdh = 0;
	uintptr_t rdt = E1000_REG_ADDR(e1000, E1000_RDT);
	*(uint32_t *)rdt = NRXDESCS - 1;
	e1000_rdt = (uint32_t *)rdt;

	uintptr_t rctl = E1000_REG_ADDR(e1000, E1000_RCTL);
	uint32_t rflag = 0;

	rflag |= E1000_RCTL_EN;
	rflag |= E1000_RCTL_BAM;
	rflag |= E1000_RCTL_SZ_4096;
	rflag |= E1000_RCTL_BSEX;
	rflag |= E1000_RCTL_SECRC;
	*(uint32_t *)rctl = rflag;

	return 0;
}

int
e1000_put_tx_desc(const uint8_t *addr, uint32_t length, uint8_t flag)
{
	int ret;
	struct tx_desc *td_p = &tx_desc_table[*e1000_tdt];
	physaddr_t c_paddr;

	if (!(td_p->status & E1000_TXD_STAT_DD)) {
		cprintf("Tranmit descriptor ring is full.\n");
		return -E_BUSY;
	}

	ret = user_mem_phy_addr((void *)addr, &c_paddr);
	if (ret < 0)
		return -E_INVAL;

	td_p->addr = c_paddr;
	td_p->length = length;
	td_p->cmd = flag | E1000_TXD_CMD_RS;

	/* Tail Pointer increase 1 */
	cprintf("e1000: index:%d addr:0x%08llx len:%d cmd:%02x (%s %d)\n",
			*e1000_tdt, td_p->addr, td_p->length, td_p->cmd,
			__FILE__, __LINE__);
	*e1000_tdt = (*e1000_tdt + 1) & (NTXDESCS - 1);
	return 0;
}

int
e1000_get_rx_desc(uint8_t *addr, uint32_t length)
{
	int index = (*e1000_rdt + 1) & (NRXDESCS - 1);
	struct rx_desc *rd_p = &rx_desc_table[index];

	if (!(rd_p->status & E1000_RXD_STAT_DD))
		return -E_EOF;

	cprintf("e1000: index:%d addr:0x%08llx len:%d sts:%02x (%s %d)\n",
			index, rd_p->addr, rd_p->length, rd_p->status,
			__FILE__, __LINE__);

	length = MIN(rd_p->length, length);
	memcpy(addr, KADDR(rd_p->addr), length);

	rd_p->status = 0;
	*e1000_rdt = index;
	return length;
}