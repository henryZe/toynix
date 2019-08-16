#include <lib.h>

enum {
	MAXMALLOC = 1024 * 1024,    /* max size of one allocated chunk */
};

static void *base = NULL;

struct m_block {
	size_t size;
	struct m_block *prev;
	struct m_block *next;
	int free;
	void *ptr;
	uint8_t data[0];
};

#define BLOCK_SIZE sizeof(struct m_block)
#define align4(x) ((((x) + 3) >> 2) << 2)

static struct m_block*
find_block(struct m_block **last, size_t s)
{
	struct m_block *b = base;
	/* first fit algorithm */
	while (b && !(b->free && b->size >= s)) {
		*last = b;
		b = b->next;
	}

	return b;
}

/* Split block according to size */
static void
split_block(struct m_block *b, size_t s)
{
	if (!b)
		return;

	struct m_block *new = (struct m_block *)(b->data + s);
	new->size = b->size - s - BLOCK_SIZE;
	new->next = b->next;
	new->prev = b;
	new->free = 1;
	new->ptr = new->data;

	b->size = s;
	b->next = new;

	if (new->next)
		new->next->prev = new;
}

/* Add a new block at the of heap */
static struct m_block*
extend_heap(struct m_block *last, size_t s)
{
	struct m_block *b = sbrk(0);

	if (sbrk(BLOCK_SIZE + s) == (void *)-1)
		return NULL;

	b->size = s;
	b->next = NULL;
	b->prev = last;
	b->ptr = b->data;
	b->free = 0;
	if (last)
		last->next = b;

	return b;
}

void *
malloc(size_t n)
{
	size_t s = align4(n);
	struct m_block *b, *last;

	if (n >= MAXMALLOC)
		return NULL;

	if (base) {
		/* First find a block */
		last = base;
		b = find_block(&last, s);
		if (b) {
			/* can we split */
			if ((b->size - s) >= (BLOCK_SIZE + 4))
				split_block(b, s);
			b->free = 0;
		} else {
			/* No fitting block, extend the heap */
			b = extend_heap(last, s);
			if (!b)
				return NULL;
		}
	} else {
		/* first time */
		b = extend_heap(NULL, s);
		if (!b)
			return NULL;
		base = b;		
	}

	return b->data;	
}



// void
// free(void *v)
// {
// 	uint8_t *c;
// 	uint32_t *ref;

// 	if (v == NULL)
// 		return;

// 	assert(mbegin <= (uint8_t *)v && (uint8_t *)v < mend);

// 	c = ROUNDDOWN(v, PGSIZE);

// 	while (uvpt[PGNUM(c)] & PTE_CONTINUED) {
// 		sys_page_unmap(0, c);
// 		c += PGSIZE;
// 		assert(mbegin <= c && c < mend);
// 	}

// 	/*
// 	 * c is just a piece of this page, so dec the ref count
// 	 * and maybe free the page.
// 	 */
// 	ref = (uint32_t *)(c + PGSIZE - 4);
// 	if (--(*ref) == 0)
// 		sys_page_unmap(0, c);
// }
