#include <lib.h>
#include <malloc.h>

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
	struct m_block *b, *last;
	size_t s = align4(n);

	if (n >= MAXMALLOC)
		return NULL;

	if (base) {
		/* First find a block */
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

/* Get the block from addr */
static struct m_block*
get_block(void *p)
{
	uint8_t *temp = p;
	temp -= BLOCK_SIZE;
	p = temp;
	return p;
}

static int
valid_addr(void *v)
{
	assert(base);
	assert(base < v && v < sbrk(0));
	assert(v == get_block(v)->ptr);
	return 1;
}

static struct m_block *
fusion(struct m_block *b)
{
	if (b->next && b->next->free) {
		b->size += BLOCK_SIZE + b->next->size;
		b->next = b->next->next;

		if (b->next)
			b->next->prev = b;
	}

	return b;
}

void
free(void *v)
{
	struct m_block *b;

	if (valid_addr(v)) {
		b = get_block(v);
		b->free = 1;

		/* fusion with previous if possible */
		if (b->prev && b->prev->free)
			b = fusion(b->prev);

		/* then fusion with next */
		if (b->next)
			fusion(b);
		else {
			/* free the end of the heap */
			if (b->prev)
				b->prev->next = NULL;
			else
				/* No more block */
				base = NULL;
			sbrk(-(b->size + BLOCK_SIZE));
		}
	}
}

void *
calloc(size_t nmemb, size_t size)
{
	size_t *new;
	size_t i, s4;

	new = malloc(nmemb * size);
	if (new) {
		s4 = align4(nmemb * size) >> 2;
		for (i = 0; i < s4; i++)
			new[i] = 0;
	}

	return new;
}

/* Copy data from block to block */
static void
copy_block(struct m_block *src , struct m_block *dst)
{
	int *sdata, *ddata;
	size_t i;

	sdata = src->ptr;
	ddata = dst->ptr;
	for (i = 0; (i << 2) < src->size && (i << 2) < dst->size; i++)
		ddata[i] = sdata[i];
}

void *
realloc(void *ptr, size_t size)
{
	size_t s;
	struct m_block *b, *new;
	void *newp;

	if (!ptr)
		return malloc(size);

	if (valid_addr(ptr)) {
		s = align4(size);
		b = get_block(ptr);

		if (b->size >= s) {
			if (b->size - s >= BLOCK_SIZE + 4)
				split_block(b, s);
		} else {
			/* Try fusion with next if possible */
			if (b->next && b->next->free
				&& (b->size + BLOCK_SIZE + b->next->size) >= s) {
				fusion(b);
				if (b->size - s >= BLOCK_SIZE + 4)
					split_block(b, s);
			} else {
				/* realloc with a new block */
				newp = malloc(s);
				if (!newp)
					return NULL;

				new = get_block(newp);
				/* Copy data */
				copy_block(b, new);
				/* free the old one */
				free(ptr);

				return newp;
			}
		}
		return ptr;
	}
	return NULL;
}
