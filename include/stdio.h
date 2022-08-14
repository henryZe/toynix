#ifndef INC_STDIO_H
#define INC_STDIO_H

#include <stdarg.h>
#include <compiler_attributes.h>

#ifndef NULL
#define NULL	((void *)0)
#endif /* !NULL */

// lib/console.c
void cputchar(int c);
int getchar(void);
int iscons(int fd);

// lib/printfmt.c
void vprintfmt(void (*putch)(int, void *), void *putdat, const char *fmt, va_list ap);
int vsnprintf(char *str, int size, const char *fmt, va_list ap);
// __printf(string-index, first-to-check)
__printf(3, 4)
void printfmt(void (*putch)(int, void *), void *putdat, const char *fmt, ...);
__printf(3, 4)
int snprintf(char *str, int size, const char *fmt, ...);

// lib/printf.c
__printf(1, 2)
int cprintf(const char *fmt, ...);
int vcprintf(const char *fmt, va_list ap);

// lib/fprintf.c
__printf(1, 2)
int printf(const char *fmt, ...);
__printf(2, 3)
int fprintf(int fd, const char *fmt, ...);
int vfprintf(int fd, const char *fmt, va_list ap);

// lib/readline.c
char *readline(const char *prompt);

#endif /* !INC_STDIO_H */
