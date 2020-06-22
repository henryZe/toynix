#include <lib.h>
#include <args.h>

#define BUFSIZ 1024		/* Find the buffer overrun bug! */
#define MAXARGS 16

int debug = 0;

static void
usage(void)
{
	cprintf("usage: sh [-dix] [command-file]\n");
	exit();
}

// Get the next token from string s.
// Set *p1 to the beginning of the token and *p2 just past the token.
// Returns
//	0 for end-of-string;
//	< for <;
//	> for >;
//	| for |;
//	w for a word.
//
// Eventually (once we parse the space where the \0 will go),
// words get nul-terminated.
#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

/*
 * |t(symbols) |
 * p1              p2
 *
 * |w(word) ... |
 * p1              p2
 */
static int
_gettoken(char *s, char **p1, char **p2)
{
	int t;

	if (s == 0) {
		if (debug > 1)
			cprintf("GETTOKEN NULL\n");
		return 0;
	}

	if (debug > 1)
		cprintf("GETTOKEN: %s\n", s);

	*p1 = 0;
	*p2 = 0;

	while (strchr(WHITESPACE, *s))
		*s++ = 0;
	if (*s == 0) {
		if (debug > 1)
			cprintf("EOL\n");
		return 0;
	}
	if (strchr(SYMBOLS, *s)) {
		t = *s;
		*p1 = s;
		*s++ = 0;
		*p2 = s;
		if (debug > 1)
			cprintf("TOK %c\n", t);
		return t;	/* return token */
	}
	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s))
		s++;
	*p2 = s;
	if (debug > 1) {
		t = **p2;	/* for temp */
		**p2 = 0;	/* for print */
		cprintf("WORD: %s\n", *p1);
		**p2 = t;
	}
	return 'w';
}

// gettoken(s, 0) prepares gettoken for subsequent calls and returns 0.
// gettoken(0, token) parses a shell token from the previously set string,
// null-terminates that token, stores the token pointer in '*token',
// and returns a token ID (0, '<', '>', '|', or 'w').
// Subsequent calls to 'gettoken(0, token)' will return subsequent
// tokens from the string.
static int
gettoken(char *s, char **p1)
{
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}

	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

// Parse a shell command from string 's' and execute it.
// Do not return until the shell command is finished.
// runcmd() is called in a forked child,
// so it's OK to manipulate file descriptor state.
static void
runcmd(char *s)
{
	char *argv[MAXARGS], *t, argv0buf[BUFSIZ];
	int argc, c, i, ret, p[2], fd, pipe_child;

	pipe_child = 0;
	gettoken(s, 0);

again:
	argc = 0;

	while (1) {
		switch ((c = gettoken(NULL, &t))) {
		case 'w':	// Add an argument
			if (argc == MAXARGS) {
				cprintf("too many arguments\n");
				exit();
			}

			argv[argc++] = t;
			break;

		case '<':	// Input redirection
			// Grab the filename from the argument list
			if (gettoken(NULL, &t) != 'w') {
				cprintf("syntax error: < not followed by word\n");
				exit();
			}

			// Open 't' for reading as file descriptor 0
			// (which environments use as standard input).
			// We can't open a file onto a particular descriptor,
			// so open the file as 'fd',
			// then check whether 'fd' is 0.
			// If not, dup 'fd' onto file descriptor 0,
			// then close the original 'fd'.
			fd = open(t, O_RDONLY);
			if (fd < 0) {
				cprintf("open %s for read: %e", t, fd);
				exit();
			}

			if (fd != 0) {
				dup(fd, 0);
				close(fd);
			}
			break;

		case '>':	// Output redirection
			// Grab the filename from the argument list
			if (gettoken(NULL, &t) != 'w') {
				cprintf("syntax error: > not followed by word\n");
				exit();
			}

			fd = open(t, O_WRONLY | O_CREAT);
			if (fd < 0) {
				cprintf("open %s for write: %e", t, fd);
				exit();
			}

			if (fd != 1) {
				dup(fd, 1);
				close(fd);
			}
			break;

		case '|':	//pipe
			ret = pipe(p);
			if (ret < 0) {
				cprintf("pipe: %e", ret);
				exit();
			}

			if (debug)
				cprintf("PIPE: %d %d\n", p[0], p[1]);

			ret = fork();
			if (ret < 0) {
				cprintf("fork: %e", ret);
				exit();
			}

			if (ret == 0) {
				/* child */
				/* redirect input */
				if (p[0] != 0) {
					dup(p[0], 0);
					close(p[0]);
				}
				close(p[1]);
				goto again;
			} else {
				/* parent */
				pipe_child = ret;

				/* redirect output */
				if (p[1] != 1) {
					dup(p[1], 1);
					close(p[1]);
				}
				close(p[0]);
				goto runit;
			}
			panic("| not implemented");
			break;

		case '&':
			/* shell no need to wait this child process */
			if (fork())
				goto again;

			/* child runcmd */
			goto runit;

		case ';':
			ret = fork();
			if (ret == 0)
				goto runit;

			/* shell need to wait this child process */
			wait(ret);
			goto again;

		case 0:		// String is complete
			// Run the current command
			goto runit;

		default:
			panic("bad return %d from gettoken", c);
			break;
		}
	}

runit:
	// Return immediately if command line was empty.
	if (argc == 0) {
		if (debug)
			cprintf("EMPTY COMMAND\n");
		return;
	}

	// Clean up command line.
	// Read all commands from the filesystem: add an initial '/' to
	// the command name.
	// This essentially acts like 'PATH=/'.
	if (argv[0][0] != '/') {
		argv0buf[0] = '/';
		strcpy(argv0buf + 1, argv[0]);
		argv[0] = argv0buf;
	}
	argv[argc] = NULL;

	// Print the command.
	if (debug) {
		cprintf("[%08x] SPAWN:", thisenv->env_id);
		for (i = 0; argv[i]; i++)
			cprintf(" %s", argv[i]);
		cprintf("\n");
	}

	// Spawn the command!
	ret = spawn(argv[0], (const char **)argv);
	if (ret < 0)
		cprintf("spawn %s: %e\n", argv[0], ret);

	// close all file descriptors
	close_all();

	// In the parent, wait for the spawned command to exit.
	if (ret >= 0) {
		if (debug)
			cprintf("[%08x] WAIT %s %08x\n", thisenv->env_id, argv[0], ret);

		wait(ret);

		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

	// If we were the left-hand part of a pipe,
	// wait for the right-hand part to finish.
	if (pipe_child) {
		if (debug)
			cprintf("[%08x] WAIT pipe_child %08x\n", thisenv->env_id, pipe_child);

		wait(pipe_child);

		if (debug)
			cprintf("[%08x] wait finished\n", thisenv->env_id);
	}

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
	}

	if (argc > 2)
		usage();

	if ((ret = chdir("/")) < 0)
		panic("chdir: %e", ret);

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
		char temp[MAXNAMELEN];

		snprintf(temp, sizeof(temp), "%s$ ", thisenv->currentpath);
		/* read input until '\n' */
		buf = readline(inter_active ? temp : NULL);
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

		// Clumsy but will have to do for now.
		// Chdir has no effect on the parent if run in the child.
		if (buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ') {
			if (chdir(buf + 3) < 0)
				printf("cannot cd %s\n", buf + 3);
			continue;
		}

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
