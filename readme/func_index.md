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
3. initialize memory ([mem_init](#Memory-Management))
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

## Monitor

file: monitor.c

### Trace of Stack

function: mon_backtrace
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

function: env_destroy
> Free specific environment

1. set env status as ENV_DYING
2. in next schedule, call env_free
3. schedule

function: env_free
> Free the env and all memory it uses

1. switch page dir
2. remove pages and page tables of user land
3. free the page dir
4. set env free and add it into free list

## Trap

### Initialize

file: trap.c

function: trap_init
> set trap, soft interrupt, interrupt handler, and initialize TSS for each CPU, load tss selector & idt

1. set handler in idt
2. load tss & idt in each CPU

### Exception & Interruption

file: trapentry.S

function: TRAPHANDLER_NOEC TRAPHANDLER
> It pushes a trap number onto the stack, then jumps to alltraps

~~~
    save the user running state

    cross rings case:
    +--------------------+ KSTACKTOP
    | 0x00000 | old SS   |     " - 4
    |      old ESP       |     " - 8
    |     old EFLAGS     |     " - 12
    | 0x00000 | old CS   |     " - 16
    |      old EIP       |     " - 20
    |     error code     |     " - 24
    +--------------------+ <---- ESP
    |      trap num      |     " - 28
    +--------------------+

    non-cross rings case:
    +--------------------+ <---- old ESP
    |     old EFLAGS     |     " - 4
    | 0x00000 | old CS   |     " - 8
    |      old EIP       |     " - 12
    +--------------------+
~~~

function: alltraps
> generate struct Trapframe and call trap with argument Trapframe

~~~
    +--------------------+ KSTACKTOP
    | 0x00000 | old SS   |     " - 4
    |      old ESP       |     " - 8
    |     old EFLAGS     |     " - 12
    | 0x00000 | old CS   |     " - 16
    |      old EIP       |     " - 20
    |     error code     |     " - 24
    +--------------------+
    |      trap num      |     " - 28
    +--------------------+
    |       old DS       |
    |       old ES       |
    |  general registers |
    +--------------------+ <---- ESP
~~~

file: trap.c

function: trap
> handle the exception/interrupt

1. lock kernel if this CPU is halted before
2. lock kernel if this task comes from user land
3. dispatch based on the type of trap (trap_dispatch)
4. schedule

function: trap_dispatch
> dispatch based on trap num

1. page fault
2. breakpoint & debug
3. system call
4. timer
5. spurious
6. key board
7. serial port

### Page Fault

function: page_fault_handler
> handle page fault

!!!!!!!!!!

### System Call

function: syscall
> dispatches to the correct kernel function

* Console

1. sys_cputs: print string
2. sys_cgetc: read a character from the system console

* Env

1. sys_getenvid: returns the current environment's envid
2. sys_env_destroy: [env_destroy](#Create-New-Environment)
3. sys_yield: schedule !!!
4. sys_exofork: !!!
5. sys_env_set_status: set the status of a specified environment (ENV_RUNNABLE or ENV_NOT_RUNNABLE)
6. sys_env_set_trapframe: set env's eip & esp (enable interrupts, set IOPL as 0)

* Memory

1. sys_page_alloc: allocate a page of memory and map it at 'va' with permission
2. sys_page_map: map the page of memory at 'src va' in src env's address space at 'dst va' in dst env's address space with permission 'perm'
3. sys_page_unmap: unmap the page of memory at 'va' in the address space of 'env'
4. sys_env_set_pgfault_upcall: [set the page fault upcall](#Page-Fault)

* IPC

1. sys_ipc_try_send: !!!
2. sys_ipc_recv: !!!

* Time

1. sys_time_msec: gain time (unit: millisecond)

* Debug

1. sys_debug_info: gain info of CPU & memory

* Network

1. sys_tx_pkt: transmit packet to e1000
2. sys_rx_pkt: receive packet from e1000

* File System

1. sys_chdir: switch working dir of current env

### Timer

1. increase time tick (if this CPU is boot one)
2. acknowledge interrupt
3. schedule
