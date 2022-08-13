#ifndef KERN_E1000_H
#define KERN_E1000_H

#include <kernel/pci.h>

#define E1000_TCTL     0x00400  /* TX Control - RW */
#define E1000_TIPG     0x00410  /* TX Inter-packet gap -RW */
#define E1000_TDBAL    0x03800  /* TX Descriptor Base Address Low - RW */
#define E1000_TDBAH    0x03804  /* TX Descriptor Base Address High - RW */
#define E1000_TDLEN    0x03808  /* TX Descriptor Length - RW */
#define E1000_TDH      0x03810  /* TX Descriptor Head - RW */
#define E1000_TDT      0x03818  /* TX Descripotr Tail - RW */

#define E1000_RCTL     0x00100  /* RX Control - RW */
#define E1000_RDBAL    0x02800  /* RX Descriptor Base Address Low - RW */
#define E1000_RDBAH    0x02804  /* RX Descriptor Base Address High - RW */
#define E1000_RDLEN    0x02808  /* RX Descriptor Length - RW */
#define E1000_RDH      0x02810  /* RX Descriptor Head - RW */
#define E1000_RDT      0x02818  /* RX Descriptor Tail - RW */
#define E1000_RAL      0x05400  /* Receive Address - RW Array */
#define E1000_RAH      0x05404  /* Receive Address - RW Array */

/* Transmit Control */
#define E1000_TCTL_EN        (1 << 1)     /* enable tx */
#define E1000_TCTL_PSP       (1 << 3)     /* pad short packets */
#define E1000_TCTL_COLD(x)   ((x) << 12)  /* collision distance */

#define E1000_TXD_STAT_DD    (1 << 0)     /* Descriptor Done */

#define E1000_TXD_CMD_RS     (1 << 3)     /* Report Status */
#define E1000_TXD_CMD_EOP    (1 << 0)     /* End of Packet */

#define E1000_RCTL_EN        (1 << 1)     /* Receiver Enable */
#define E1000_RCTL_BAM       (1 << 15)    /* broadcast enable */
#define E1000_RCTL_SZ_4096   (3 << 16)    /* rx buffer size 4096 */
#define E1000_RCTL_BSEX      (1 << 25)    /* Buffer Size Extension */
#define E1000_RCTL_SECRC     (1 << 26)    /* Strip Ethernet CRC */

#define E1000_RAH_AV         (1 << 31)    /* Address Valid */

#define E1000_RXD_STAT_DD    (1 << 0)     /* Descriptor Done */

/*
 * transmit descriptor:
 *
 * 63            48 47   40 39   32 31   24 23   16 15             0
 * +---------------------------------------------------------------+
 * |                         Buffer address                        |
 * +---------------+-------+-------+-------+-------+---------------+
 * |    Special    |  CSS  | Status|  Cmd  |  CSO  |    Length     |
 * +---------------+-------+-------+-------+-------+---------------+
 */
struct tx_desc {
	uint64_t addr;
	uint16_t length;
	uint8_t	cso;
	uint8_t cmd;
	uint8_t	status;
	uint8_t	css;
	uint16_t special;
};

struct rx_desc {
	uint64_t addr;
	uint16_t length;
	uint16_t checksum;
	uint8_t	status;
	uint8_t	errors;
	uint16_t special;
};

int pci_e1000_attach(struct pci_func *pcif);
int e1000_put_tx_desc(const uint8_t *addr, uint32_t length, uint8_t flag);
int e1000_get_rx_desc(uint8_t *addr, uint32_t length);

#endif // KERN_E1000_H
