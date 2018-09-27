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

/* Transmit Control */
#define E1000_TCTL_EN        (1 << 1)     /* enable tx */
#define E1000_TCTL_PSP       (1 << 3)     /* pad short packets */
#define E1000_TCTL_COLD(x)   (x << 12)    /* collision distance */

#define E1000_TXD_STAT_DD    0x00000001 /* Descriptor Done */

int pci_e1000_attach(struct pci_func *pcif);

#endif // KERN_E1000_H