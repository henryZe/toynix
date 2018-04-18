#include <string.h>
#include <stdio.h>
#include <x86.h>
#include <kernel/console.h>
#include <kernel/monitor.h>
#include <kernel/pmap.h>
#include <kernel/env.h>
#include <kernel/trap.h>

void
init(void)
{
	extern char edata[], end[];

	/*
	* Before doing anything else, complete the ELF loading process.
	* Clear the uninitialized global data (BSS) section of our program.
	* This ensures that all static/global variables start out zero.
	*/
	memset(edata, 0, end - edata);

	/*
	* Initialize the console.
	* Can't call printf until after we do this!
	*/
	cons_init();
	cprintf("Enter toynix...\n");

	mem_init();

	env_init();
	trap_init();

	ENV_CREATE(user_divzero, ENV_TYPE_USER);

	// We only have one user environment for now, so just run it.
	cprintf("Enter envs[0]...\n");
	env_run(&envs[0]);
}
