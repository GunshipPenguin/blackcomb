#ifndef __STRING_H
#define __STRING_H

#include <stddef.h>

size_t strlen(const char *);
size_t strnlen(const char *, size_t);
char *strcpy(char *dest, const char *src);
size_t strlcpy(char *dest, const char *src, size_t sz);

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);

#endif /* __STRING_H */
