#include <lib.h>
#include <malloc.h>
#include <arch/thread.h>
#include <arch/threadq.h>

static thread_id_t max_tid;
static struct thread_context *cur_tc;

static struct thread_queue thread_queue;	/* FIFO */
static struct thread_queue kill_queue;

void
thread_init(void)
{
	threadq_init(&thread_queue);
	max_tid = 0;
}

static void
thread_set_name(struct thread_context *tc, const char *name)
{
	strncpy(tc->tc_name, name, name_size - 1);
	tc->tc_name[name_size - 1] = 0;
}

static thread_id_t
alloc_tid(void)
{
	int tid = max_tid++;

	if (max_tid == (uint32_t)~0)
		panic("alloc_tid: no more thread ids");

	return tid;
}

static void
thread_clean(struct thread_context *tc)
{
	if (!tc)
		return;

	int i;

	for (i = 0; i < tc->tc_nonhalt; i++)
		tc->tc_onhalt[i](tc->tc_tid);

	free(tc->tc_stack_bottom);
	free(tc);
}

void
thread_yield(void)
{
	struct thread_context *next_tc = threadq_pop(&thread_queue);

	if (!next_tc)
		return;

	if (cur_tc) {
		if (toynix_setjmp(&cur_tc->tc_jb) != 0)
			/* means this thread returns again */
			return;

		/* push self into thread queue */
		threadq_push(&thread_queue, cur_tc);
	}

	cur_tc = next_tc;
	/* never return, and jump to setjmp */
	toynix_longjmp(&cur_tc->tc_jb, 1);
}

void
thread_halt(void)
{
	// right now the kill_queue will never be more than one
	// clean up a thread if one is on the queue
	thread_clean(threadq_pop(&kill_queue));

	threadq_push(&kill_queue, cur_tc);
	cur_tc = NULL;
	thread_yield();

    // WHAT IF THERE ARE NO MORE THREADS? HOW DO WE STOP?
    // when yield has no thread to run, it will return here!
	exit();
}

static void
thread_entry(void)
{
	cur_tc->tc_entry(cur_tc->tc_arg);
	thread_halt();	/* thread exit */
}

int
thread_create(thread_id_t *tid, const char *name,
				void (*entry)(uint32_t), uint32_t arg)
{
	struct thread_context *tc = malloc(sizeof(struct thread_context));
	if (!tc)
		return -E_NO_MEM;

	memset(tc, 0, sizeof(struct thread_context));

	thread_set_name(tc, name);
	tc->tc_tid = alloc_tid();

	tc->tc_stack_bottom = malloc(stack_size);
	if (!tc->tc_stack_bottom) {
		free(tc);
		return -E_NO_MEM;
	}

	void *stacktop = tc->tc_stack_bottom + stack_size;
	stacktop -= 4;
	memset(stacktop, 0, 4);

	memset(&tc->tc_jb, 0, sizeof(tc->tc_jb));
	tc->tc_jb.jb_esp = (uint32_t)stacktop;
	tc->tc_jb.jb_eip = (uint32_t)&thread_entry;
	tc->tc_entry = entry;
	tc->tc_arg = arg;

	threadq_push(&thread_queue, tc);

	if (tid)
		*tid = tc->tc_tid;

	return 0;
}



