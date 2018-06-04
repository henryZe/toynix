#include <lib.h>

const char *str1 = "hello child environment! how are you?";
const char *str2 = "hello parent environment! I'm good.";

#define TEMP_ADDR	((char *)0xa00000)
#define TEMP_ADDR_CHILD	((char *)0xb00000)

void
umain(int argc, char **argv)
{
	envid_t who;

	who = fork();
	if (who == 0) {
		/* child */
		ipc_recv(&who, TEMP_ADDR_CHILD, NULL);
		cprintf("%x got message: %s\n", who, TEMP_ADDR_CHILD);
		if (strncmp(TEMP_ADDR_CHILD, str1, strlen(str1)) == 0)
			cprintf("child received correct message\n");

		memcpy(TEMP_ADDR_CHILD, str2, strlen(str2));
		ipc_send(who, 0, TEMP_ADDR_CHILD, PTE_W);
		return;
	}

	/* parent */
	sys_page_alloc(0, TEMP_ADDR, PTE_W);
	memcpy(TEMP_ADDR, str1, strlen(str1));
	ipc_send(who, 0, TEMP_ADDR, PTE_W);

	ipc_recv(&who, TEMP_ADDR, NULL);
	cprintf("%x got message: %s\n", who, TEMP_ADDR);
	if (strncmp(TEMP_ADDR, str2, strlen(str2)) == 0)
		cprintf("parent received correct message\n");

	return;
}
