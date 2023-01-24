#include "string.h"

size_t strnlen(const char *s, size_t maxlen)
{
    size_t len = 0;
    while (s[len] != '\0' && len < maxlen)
        len++;

    return len;
}

size_t strlen(const char *s)
{
    size_t len = 0;
    while (s[len])
        len++;

    return len;
}

char *strcpy(char *dest, const char *src)
{
    for (size_t i = 0; i < strlen(src); i++)
        dest[i] = src[i];
    return dest;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }

    return *((const unsigned char *)s1) - *((const unsigned char *)s2);
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }

    return n == 0 ? 0 : *((const unsigned char *)s1) - *((const unsigned char *)s2);
}

void *memset(void *s, int c, size_t n)
{
    for (size_t i = 0; i < n; i++)
        ((char *)s)[i] = c;

    return s;
}

void *memcpy(void *dest, const void *src, size_t n)
{
    for (size_t i = 0; i < n; i++)
        ((char *)dest)[i] = ((char *)src)[i];

    return dest;
}
