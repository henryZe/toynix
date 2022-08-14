/* See COPYRIGHT for copyright information. */

#ifndef INC_ASSERT_H
#define INC_ASSERT_H

#include <stdio.h>
#include <compiler_attributes.h>

#define warn(...) _warn(__FILE__, __LINE__, __VA_ARGS__)
#define panic(...) _panic(__FILE__, __LINE__, __VA_ARGS__)

void _warn(const char *file, int line, const char *fmt, ...);
void __noreturn _panic(const char *file, int line, const char *fmt, ...);

#define assert(x)	\
	do { if (!(x)) panic("assertion failed: %s", #x); } while (0)

// static_assert(x) will generate a compile-time error if 'x' is false.
#define static_assert(x)	{ switch (x) case 0: case (x): ; }

void __stack_chk_fail(void);

#endif /* !INC_ASSERT_H */
