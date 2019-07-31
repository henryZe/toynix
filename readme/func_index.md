# Toynix Index

## Boot Flow

### x86 boot ROM

start pa: 0xffff0

### Bootloader

start pa: 0x7c00

file: boot.S function: start

1. switch to 32-bit protected mode
2. jump into bootmain

file: bootmain.c function: bootmain

1. load elf file, include elf header & program headers
2. jump into kernel entry

### Kernel

start pa: 0x00100000 va: 0xF0100000

file: entry.S function: _start

1. load entry_pgdir into cr3 and turn on paging
2. initialize kernel stack(8 pages) in data segment
3. call init

file: init.c function: init

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

## Stack

file: monitor.c mon_backtrace
> Display the trace of function call

1. read & load ebp
2. gain eip according to ebp
3. parse eip from STAB file (kernel/user, file, function, line)

~~~
            +---------------+
            |   arg n       |
            +---------------+
            |   arg 2       |
            +---------------+
            |   arg 1       |
            +---------------+
            |   ret %eip    | caller function's stack frame
            +===============+
            |   saved %ebp  | callee function's stack frame
%ebp -->    +---------------+ <-- where Prologue & Epilogue happen
            |   local       |
            |   variables,  |
%esp -->    |   etc.        |
            +---------------+
~~~

## Memory Management

### Initialize

file: pmap.c function: mem_init

1. detect available physical memory size
2. allocate kern_pgdir by simplified allocator(boot_alloc)
3. setup user page tables (UVPT, read only)
4. allocate page array
5. allocate env array
6. initialize page array
7. map page array to user (UPAGES, read only)
8. map env array to user (UENVS, read only)
9. map [KERNBASE, 2^32) to kernel
10. map kernel stack array to kernel (KSTACKTOP)
11. load kern_pgdir into cr3 register
12. enable paging by loading cr0 register

### Page Management

file: pmap.c

function: page_alloc
> Allocates a physical page

function: page_free
> Return a page to the free list

function: page_lookup
> Return the page mapped at specific virtual address

function: page_insert
> Map the physical page at specific virtual address

function: page_remove
> Unmap the physical page at specific virtual address

1. look up physical page by va
2. decrease page ref count
3. invalidate TLB entry

function: user_mem_check
> Check that an environment is allowed to access the range of memory with specific permission

## Environment

### Initialize

file: pmap.c

function: env_init
> Initialize all of the Env structures in the envs array and add them to the env_free_list

function: env_init_percpu
> Configure the segmentation hardware with separate segments for privilege level 0 (kernel) and privilege level 3 (user)

### Create New Environment

function: env_setup_vm
> Initialize the kernel virtual memory layout for environment e

function: region_alloc
> Allocate len bytes of physical memory for environment env and map it at va

function: load_icode
> Parse an ELF binary image and load its contents into the user address space

1. switch env_pgdir
2. load ELF image
3. set tf_eip
4. initial stack by mapping one page
5. switch back to kern_pgdir

function: env_alloc
> Allocates and initializes a new environment

1. call env_setup_vm
2. initialize Trapframe register

function: env_create
> Allocate an environment with env_alloc and call load_icode to load an ELF binary into it

function: env_run
> Start a given environment running in user mode

1. set current env as spending
2. set target env as running
3. switch env_pgdir
4. unlock kernel before returning to user
5. call env_pop_tf

function: env_pop_tf
> Restores the register values in the Trapframe with the 'iret' instruction

## Trap
