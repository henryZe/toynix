#include <lib.h>

const char *msg = "This is the NEW message of the day!\n\n";

#define FVA ((struct Fd *)0xCCCCC000)

static int
xopen(const char *path, int mode)
{

}

void
umain(int argc, char **argv)
{
	int ret, f, i;
	struct Fd *fd;
	struct Fd fdcopy;
	struct Stat st;
	char buf[512];

	/* we open files manually first, to avoid the FD layer */
	ret = xopen();

}