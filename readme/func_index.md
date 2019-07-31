# Bootloader

## file: boot.S

### function: start

1. switch to 32-bit protected mode
2. jump into bootmain

## file: bootmain.c

### function: bootmain

1. load elf file, include elf header & program headers
2. jump into kernel entry

# Kernel

## file: entry.S

### function: entry

1. load entry_pgdir into cr3 and turn on paging
2. initialize kernel stack(8 pages) in data segment
3. call init

## file: init.c

### function: init

1. initialize bss segment
2. initialize console devices including CGA, keyboard and serial port (cons_init)
3. initialize memory ([mem_init](#Memory-Manage))
4. initialize task ([env_init](#Environment))
5. initialize trap ([trap_init](#Trap))
6. initialize multiprocessor (mp_init)
7. initialize interrupt controller (lapic_init & pic_init)
8. initialize clock (time_init)
9. initialize PCI bus (pci_init)
10. lock kernel before waking up non-boot CPUs (boot_aps)
11. start file system server (fs serve)
12. start network server (net serve)
13. start init process (user initsh)

# Memory Manage

## file: pmap.c

### function: mem_init


# Environment

# Trap