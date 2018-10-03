#include <lib.h>
#include <malloc.h>
#include <lwip/sys.h>
#include <arch/queue.h>
#include <arch/sys_arch.h>
#include <arch/thread.h>
#include <arch/perror.h>

#define debug 0

#define NSEM		256
#define NMBOX		128
#define MBOXSLOTS	32

struct sys_sem_entry {
	int freed;
	int gen;
	union {
		uint32_t v;
		struct {
			uint16_t counter;
			uint16_t waiters;	/* whether there is waiter */
		};
	};
	LIST_ENTRY(sys_sem_entry) link;
};

static struct sys_sem_entry sems[NSEM];
static LIST_HEAD(sem_list, sys_sem_entry) sem_free;

struct sys_mbox_entry {
	int freed;
	int head, nextq;
	void *msg[MBOXSLOTS];
	sys_sem_t queued_msg;
	sys_sem_t free_msg;
	LIST_ENTRY(sys_mbox_entry) link;
};

static struct sys_mbox_entry mboxes[NMBOX];
static LIST_HEAD(mbox_list, sys_mbox_entry) mbox_free;

void
sys_init(void)
{
	int i = 0;

	for (i = 0; i < NSEM; i++) {
		sems[i].freed = 1;
		LIST_INSERT_HEAD(&sem_free, &sems[i], link);
	}

	for (i = 0; i < NMBOX; i++) {
		mboxes[i].freed = 1;
		LIST_INSERT_HEAD(&mbox_free, &mboxes[i], link);
	}
}

sys_sem_t
sys_sem_new(uint8_t count)
{
	struct sys_sem_entry *sema = LIST_FIRST(&sem_free);
	if (!sema) {
		cprintf("lwip: sys_sem_new: out of semaphores\n");
		return SYS_SEM_NULL;
	}

	LIST_REMOVE(sema, link);
	assert(sema->freed);
	sema->freed = 0;

	sema->counter = count;
	sema->gen++;
	return sema - &sems[0];
}

void
sys_sem_free(sys_sem_t sem)
{
	assert(!sems[sem].freed);
	sems[sem].freed = 1;
	sems[sem].gen++;
	LIST_INSERT_HEAD(&sem_free, &sems[sem], link);
}

void
sys_sem_signal(sys_sem_t sem)
{
	assert(!sems[sem].freed);
	sems[sem].counter++;
	if (sems[sem].waiters) {
		sems[sem].waiters = 0;
		thread_wakeup(&sems[sem].v);
	}
}

uint32_t
sys_arch_sem_wait(sys_sem_t sem, uint32_t tm_msec)
{
	assert(!sems[sem].freed);

	uint32_t waited = 0;
	int gen = sems[sem].gen; // backup

	while (tm_msec == 0 || waited < tm_msec) {
		if (sems[sem].counter > 0) {
			sems[sem].counter--;
			return waited;

		} else if (tm_msec == SYS_ARCH_NOWAIT) {
			return SYS_ARCH_TIMEOUT;

		} else {
			uint32_t a = sys_time_msec();
			uint32_t sleep_until = tm_msec ? a + (tm_msec - waited) : ~0;
			uint32_t cur_v = sems[sem].v;

			sems[sem].waiters = 1;
			lwip_core_unlock();
			thread_wait(&sems[sem].v, cur_v, sleep_until);
			lwip_core_lock();

			if (gen != sems[sem].gen) {
				cprintf("sys_arch_sem_wait: sem freed under waiter!\n");
				return SYS_ARCH_TIMEOUT;
			}

			uint32_t b = sys_time_msec();
			waited += (b - a);
		}
	}

	return SYS_ARCH_TIMEOUT;
}

struct lwip_thread {
	void (*func)(void *arg);
	void *arg;
};

static void
lwip_thread_entry(uint32_t arg)
{
	struct lwip_thread *lt = (struct lwip_thread *)arg;

	lwip_core_lock();
	lt->func(lt->arg);
	lwip_core_unlock();

	free(lt);
}

sys_thread_t
sys_thread_new(char *name, void (* thread)(void *arg), void *arg,
				int stacksize, int prio)
{
	struct lwip_thread *lt = malloc(sizeof(*lt));
	if (!lt)
		panic("sys_thread_new: cannot allocate thread struct");

	if (stacksize > PGSIZE)
		panic("large stack %d", stacksize);

	lt->func = thread;
	lt->arg = arg;

	thread_id_t tid;
	int r = thread_create(&tid, name, lwip_thread_entry, (uint32_t)lt);

	if (r < 0)
		panic("lwip: sys_thread_new: cannot create: %s\n", e2s(r));

	return tid;
}

struct sys_timeouts
sys_arch_timeouts
{

}

void
lwip_core_lock(void)
{
	return;
}

void
lwip_core_unlock(void)
{
	return;
}