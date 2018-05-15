#include <kernel/cpu.h>

struct CpuInfo cpus[NCPU];
struct CpuInfo *bootcpu;
int ismp;
int ncpu;

// Per-CPU kernel stacks
unsigned char percpu_kstacks[NCPU][KSTKSIZE]
__attribute__ ((aligned(PGSIZE)));




















