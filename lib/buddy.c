/*
 * buddy allocator
 */

#include <buddy.h>
#include <error.h>
#include <assert.h>
#include <math.h>

#define lchild_of(x) ((((x) + 1) << 1) - 1)
#define rchild_of(x) (((x) + 1) << 1)
#define parent_of(x) ((((x) + 1) >> 1) - 1)

int buddy_init(struct buddy_pool *buddy,
		unsigned long start, size_t size,
		void *manager_area, size_t node_size)
{
	unsigned long i = 0;
	unsigned long child_order = 0;

	if ((!buddy) || (!size) || (!manager_area) || (!node_size))
		return -E_INVAL;

	if ((!is_pow_of_2(size)) || (!is_pow_of_2(node_size))) {
		cprintf("size isn't power of 2\n");
		return -E_INVAL;
	}

	if (size % node_size) {
		cprintf("size isn't aligned\n");
		return -E_INVAL;
	}

	spin_lock_init(&buddy->lock);
	buddy->start = start;
	buddy->size = size;
	buddy->order = log_of_2(size);
	buddy->curr_size = size;
	buddy->node_size = node_size;
	buddy->manager = manager_area;
	buddy->manager_size = buddy_mgs(size, node_size);

	/* prepare the manager, using log2N */
	child_order = buddy->order + 1;
	for (i = 0; i < buddy->manager_size; i++) {
		if (is_pow_of_2(i + 1))
			child_order--;
		buddy->manager[i].order = child_order;
	}

	return 0;
}


void *buddy_alloc(struct buddy_pool *buddy,
		size_t size)
{
	unsigned char order = 0;
	unsigned long i = 0;
	unsigned long offset = 0;
	unsigned long lchild_order = 0;
	unsigned long rchild_order = 0;
	unsigned long flags = 0;

	if (!buddy || !buddy->manager || !size)
		return NULL;

	size = next_pow_of_2(size);
	size = MAX(size, buddy->node_size);

	if (size > (1UL << buddy_max_order(buddy)))
		return NULL;

	order = log_of_2(size);
	flags = spin_lock_irqsave(&buddy->lock);
	while ((i < buddy->manager_size) &&
		 (buddy->manager[i].order >= order)) {
		if ((rchild_of(i) < buddy->manager_size) &&
			(buddy->manager[lchild_of(i)].order >= order))
			i = lchild_of(i);
		else
			i = rchild_of(i);
	}

	i = parent_of(i);
	offset = (i + 1) * size - buddy->size;

	if ((long)offset < 0)
		panic("buddy allocate crash\n");

	buddy->manager[i].order = 0;
	buddy->curr_size -= size;

	while (i) {
		i = parent_of(i);
		lchild_order = buddy->manager[lchild_of(i)].order;
		rchild_order = buddy->manager[rchild_of(i)].order;
		buddy->manager[i].order = MAX(lchild_order, rchild_order);
	}

	sys_spin_unlock(&buddy->lock, flags);
	return (void *)(buddy->start + offset);
}

void buddy_free(struct buddy_pool *buddy,
		void *addr)
{
	unsigned char order = 0;
	unsigned long i = 0;
	unsigned long offset = 0;
	unsigned long lchild_order = 0;
	unsigned long rchild_order = 0;
	unsigned long flags = 0;

	if ((!buddy) || (!buddy->manager) || (!addr))
		return;

	offset = (unsigned long)addr - buddy->start;
	if (offset > buddy->size)
		panic("buddy invalid addr\n");

	flags = spin_lock_irqsave(&buddy->lock);

	/* start from the youngest child */
	i = (offset + buddy->size) / buddy->node_size - 1;
	order = log_of_2(buddy->node_size);
	while (i && buddy->manager[i].order) {
		order++;
		i = parent_of(i);
	}

	if (offset != (i + 1) * (1UL << order) - buddy->size)
		panic("buddy free crash\n");

	buddy->manager[i].order = order;
	buddy->curr_size += (1UL << order);

	/* resume the parents */
	while (i) {
		i = parent_of(i);
		lchild_order = buddy->manager[lchild_of(i)].order;
		rchild_order = buddy->manager[rchild_of(i)].order;
		buddy->manager[i].order = MAX(lchild_order, rchild_order);
		if ((lchild_order == order) && (rchild_order == order))
			buddy->manager[i].order++;
		order++;
	}
	spin_unlock_irqrestore(&buddy->lock, flags);
}
