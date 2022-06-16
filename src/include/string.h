#ifndef __STRING_H
#define __STRING_H

#include <stddef.h>

size_t strlen(const char *);
size_t strnlen(const char *, size_t);
char *strcpy(char *dest, const char *src);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

#endif /* __STRING_H */
