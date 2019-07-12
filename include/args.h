#ifndef INC_ARGS_H
#define INC_ARGS_H

struct Argstate {
	int *argc;
	const char **argv;
	const char *curarg;
	const char *argvalue;
};

// Toynix command-line parsing functions.

// Initializes the Argstate buffer from argc and argv.
// (Note: it is safe to use a 'const char **' for argv.)
void argstart(int *argc, char **argv, struct Argstate *args);

// Returns the next flag in the argument list,
// or -1 if there are no more flags.
//
// Flags stop at a non-flag (anything that doesn't start with '-'),
// at the end of the argument list, before "-", or after "--",
// whichever comes first.  Any "--" argument is not returned.
//
// Consumes arguments from the argc/argv array passed in to argstart.
// If you argstart with an argc/argv array of ["sh", "-i", "foo"],
// the first call to argnext will return 'i' and change the array to
// ["sh", "foo"].  Thus, when argnext returns -1, the argc/argv array
// contains just the non-flag arguments.
int argnext(struct Argstate *);

// Example:
//
//	#include <lib.h>
//
//	void
//	umain(int argc, char **argv)
//	{
//		int i;
//		struct Argstate args;
//
//		argstart(&argc, argv, &args);
//		while ((i = argnext(&args)) >= 0)
//			switch (i) {
//			case 'r':
//			case 'x':
//				cprintf("'-%c' flag\n", i);
//				break;
//			case 'f':
//				cprintf("'-f %s' flag\n", argvalue(&args));
//				break;
//			default:
//				cprintf("unknown flag\n");
//			}
//
//		for (i = 1; i < argc; i++)
//			cprintf("argument '%s'\n", argv[i]);
//	}
//
// If this program is run with the arguments
// ["-rx", "-f", "foo", "--", "-r", "duh"]
// it will print out
//	'-r' flag
//	'-x' flag
//	'-f foo' flag
//	argument '-r'
//	argument 'duh'

#endif
