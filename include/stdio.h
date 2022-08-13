#ifndef INC_STDIO_H
#define INC_STDIO_H

#include <stdarg.h>

#ifndef NULL
#define NULL	((void *) 0)
#endif /* !NULL */

// lib/console.c
void cputchar(int c);
int	getchar(void);
int	iscons(int fd);

// lib/printfmt.c
void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list ap);
int vsnprintf(char *str, int size, const char *fmt, va_list ap);
// __attribute__((format(printf, string-index, first-to-check)))
void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...)
		__attribute__((format(printf, 3, 4)));
int snprintf(char *str, int size, const char *fmt, ...)
		__attribute__((format(printf, 3, 4)));

// lib/printf.c
int cprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int vcprintf(const char *fmt, va_list ap);

// lib/fprintf.c
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int fprintf(int fd, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int vfprintf(int fd, const char *fmt, va_list ap);

// lib/readline.c
char *readline(const char *prompt);

#endif /* !INC_STDIO_H */
