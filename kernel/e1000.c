#include <kernel/e1000.h>
#include <kernel/pmap.h>
#include <stdio.h>

volatile uint32_t *e1000;

int
pci_e1000_attach(struct pci_func *pcif)
{
	pci_func_enable(pcif);

	e1000 = mmio_map_region(pcif->reg_base[0], pcif->reg_size[0]);

	cprintf("e1000[2] = %08x\n", e1000[2]);

	return 0;
}

