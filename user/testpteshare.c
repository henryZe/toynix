#include <lib.h>
#include <x86.h>

#define VA	((char *)0xA0000000)

const char *msg = "hello, world\n";
const char *msg2 = "goodbye, world\n";

static void
child_of_spawn(void)
{
	strcpy(VA, msg2);
	exit();
}

void
umain(int argc, char **argv)
{
	int ret;

	if (argc)
		child_of_spawn();

	ret = sys_page_alloc(0, VA, PTE_W | PTE_SHARE);
	if (ret < 0)
		panic("sys_page_alloc: %e", ret);

	// check fork
	ret = fork();
	if (ret < 0)
		panic("fork: %e", ret);
	if (ret == 0) {
		strcpy(VA, msg);
		exit();
	}
	wait(ret);
	cprintf("fork handles PTE_SHARE %s\n", strcmp(VA, msg) == 0 ? "right" : "wrong");

	// check spawn
	ret = spawnl("/testpteshare", "testpteshare", NULL);	/* argc = 1 */
	if (ret < 0)
		panic("spawn: %e", ret);
	wait(ret);
	cprintf("spawn handles PTE_SHARE %s\n", strcmp(VA, msg2) == 0 ? "right" : "wrong");

	breakpoint();
}
