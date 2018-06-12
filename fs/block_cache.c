#include <lib.h>
#include <x86.h>
#include <fs.h>

void
umain(int argc, char **argv)
{
	static_assert(sizeof(struct File) == 256);
	binaryname = "fs";
	cprintf("FS is running\n");

	// Check that we are able to do I/O
	outw(0x8A00, 0x8A00);
	cprintf("FS can do I/O\n");
/*
	serve_init();
	fs_init();
	fs_test();
	serve();
*/
}
