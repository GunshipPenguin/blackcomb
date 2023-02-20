#include <stdint.h>

#include "printf.h"
#include "regs.h"

void do_syscall(const struct regs *rg)
{
    printf("syscall %d performed\n", rg->rax);
    return;
}
