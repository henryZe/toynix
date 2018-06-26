#include <lib.h>
#include <args.h>

int debug = 0;

void
usage(void)
{
	cprintf("usage: sh [-dix] [command-file]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int ret, inter_active, echo_cmds;
	struct Argstate args;

	inter_active = '?';
	echo_cmds = 0;
	argstart(&argc, argv, &args);

	/* configure shell setting: -d, -i, -x */
	while ((ret = argnext(&args)) >= 0) {
		switch (ret) {
		case 'd':
			debug++;
			break;
		case 'i':
			inter_active = 1;
			break;
		case 'x':
			echo_cmds = 1;
			break;
		default:
			usage();
		}

		if (argc > 2)
			usage();

		if (argc == 2) {
			/* close standard input */
			close(0);

			/* open script file as fd 0 */
			ret = open(argv[1], O_RDONLY);
			if (ret < 0)
				panic("open %s: %e", argv[1], ret);

			assert(ret == 0);
		}

		if (inter_active == '?')
			inter_active = iscons(0);

		while (1) {
			char *buf;

			/* read input until '\n' */
			buf = readline(inter_active ? "sh $ " : NULL);
			if (buf == NULL) {
				if (debug)
					cprintf("EXITING\n");
				exit();		// end of file
			}

			if (debug)
				cprintf("LINE: %s\n", buf);

			if (buf[0] == '#')	/* comment out */
				continue;

			if (echo_cmds)
				printf("# %s\n", buf);

			if (debug)
				cprintf("BEFORE FORK\n");

			ret = fork();
			if (ret < 0)
				panic("fork: %e", ret);
	
			if (debug)
				cprintf("FORK: %d\n", ret);

			if (ret == 0) {
				/* child */
				runcmd(buf);
				exit();
			} else
				wait(ret);
		}
	}
}
