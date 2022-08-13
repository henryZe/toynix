#include <lib.h>

static uint8_t *mbegin = (uint8_t *)0x08000000;
static uint8_t *mend   = (uint8_t *)0x10000000;
static uint8_t *heap_break;

void *
sbrk(intptr_t increment)
{
	int ret;
	uint8_t *new_break, *i;

	if (heap_break == NULL) {
		sys_add_vma(0, (unsigned long)mbegin, mend - mbegin, PTE_W);
		heap_break = mbegin;
	}

	if (!increment)
		return heap_break;

	new_break = heap_break + increment;
	if ((new_break < mbegin) || (new_break > mend))
		return (void *)-E_INVAL;

	if (new_break > heap_break) {
		for (i = ROUNDUP(heap_break, PGSIZE); i < ROUNDUP(new_break, PGSIZE); i += PGSIZE) {
			// mmap zero page
			ret = sys_page_map(0, NULL, 0, i, PTE_COW);
			if (ret < 0)
				return (void *)ret;
		}
	} else {
		for (i = ROUNDDOWN(heap_break, PGSIZE); i >= ROUNDUP(new_break, PGSIZE); i -= PGSIZE)
			sys_page_unmap(0, i);
	}

	heap_break = new_break;
	return heap_break;
}
