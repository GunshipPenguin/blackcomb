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
