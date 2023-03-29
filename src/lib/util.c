#include "util.h"
#include "printf.h"

void panic(char *s, ...)
{
    va_list args;
    va_start(args, s);

    vprintf(s, args);
    printf("\n");

    for (;;) {
    }
}
