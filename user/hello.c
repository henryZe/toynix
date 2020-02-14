#include <lib.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %8x\n", thisenv->env_id);

	for (size_t i = 0; i < thisenv->vma_valid; i++)
		cprintf("vma[%d] start %08x size %08x perm %08x\n",
			i, thisenv->vma[i].vm_start, thisenv->vma[i].size, thisenv->vma[i].vm_page_prot);
}