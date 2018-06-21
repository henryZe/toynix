#include <lib.h>

void
umain(int argc, char **argv)
{
	int ret;

	cprintf("i am parent environment %08x\n", thisenv->env_id);

	ret = spawnl("hello", "hello", NULL);
	if (ret < 0)
		panic("spawn(hello) failed: %e", ret);
}
