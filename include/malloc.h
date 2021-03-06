#ifndef INC_MALLOC_H
#define INC_MALLOC_H

#include <types.h>

void *malloc(size_t size);
void free(void *addr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#endif
