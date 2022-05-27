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
