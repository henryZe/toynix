/*
 * buddy allocator
 */

#ifndef INC_BUDDY_H
#define INC_BUDDY_H

#include <types.h>

/*
 * Get the buddy manager area size
 */
#define buddy_mgs(size, node_size) ((((size) / (node_size)) << 1) - 1)
#define buddy_max_order(b) (b->manager[0].order)

struct buddy_pool {
	unsigned long start; /* heap start */
	size_t size; /* max size */
	unsigned long order; /* max order = log_of_2(size) */
	size_t node_size; /* min alloc size */
	struct {
		unsigned char order;
	} *manager; /* manager controls the child's order */
	size_t manager_size;
	size_t curr_size;
	int lock;
};

int buddy_init(struct buddy_pool *buddy,
		unsigned long start, size_t size,
		void *manager_area, size_t node_size);

void *buddy_alloc(struct buddy_pool *buddy,
		size_t size);

void buddy_free(struct buddy_pool *buddy,
		void *addr);

#endif
