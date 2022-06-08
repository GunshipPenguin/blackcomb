#include "util.h"
#include "printf.h"

void panic(char *s)
{
    printf("%s\n", s);
    for (;;) {
    }
}
