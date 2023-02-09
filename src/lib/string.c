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
    int res = 0;
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0')
        res += s1[i] - s2[i];

    return res;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    int res = 0;
    int i = 0;
    while (s1[i] != '\0' && s2[i] != '\0' && i < n)
        res += s1[i] - s2[i];

    return res;
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
