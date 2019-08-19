#include <vm.h>
#include <error.h>
#include <env.h>

int
env_add_vma(struct Env *e, unsigned long start, uint32_t size, uint32_t perm)
{
	struct vm_area_struct *vma;

	if (e->vma_valid >= VMA_PER_ENV)
		return -E_MAX_OPEN;

	vma = &e->vma[e->vma_valid];
	vma->vm_start = start;
	vma->size = size;
	vma->vm_page_prot = perm;
	e->vma_valid++;

	return 0;
}