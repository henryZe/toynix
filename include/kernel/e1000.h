#ifndef KERN_E1000_H
#define KERN_E1000_H

#include <kernel/pci.h>

int pci_e1000_attach(struct pci_func *pcif);

#endif // KERN_E1000_H