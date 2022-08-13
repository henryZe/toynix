#include <args.h>
#include <string.h>

void
argstart(int *argc, char **argv, struct Argstate *args)
{
	args->argc = argc;
	args->argv = (const char **) argv;
	args->curarg = (*argc > 1 && argv ? "" : NULL);
	args->argvalue = 0;
}

int
argnext(struct Argstate *args)
{
	int arg;

	args->argvalue = 0;

	// Done processing arguments if args->curarg == NULL
	if (args->curarg == NULL)
		return -1;

	if (!*args->curarg) {
		// Need to process the next argument
		// Check for end of argument list
		if (*args->argc == 1
		    || args->argv[1][0] != '-'
		    || args->argv[1][1] == '\0')
			goto endofargs;
		// Shift arguments down one
		args->curarg = args->argv[1] + 1;
		memmove(args->argv + 1, args->argv + 2, sizeof(const char *) * (*args->argc - 1));
		(*args->argc)--;
		// Check for "--": end of argument list
		if (args->curarg[0] == '-' && args->curarg[1] == '\0')
			goto endofargs;
	}

	arg = (unsigned char) *args->curarg;
	args->curarg++;
	return arg;

endofargs:
	args->curarg = NULL;
	return -1;
}
