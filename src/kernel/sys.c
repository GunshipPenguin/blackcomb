#include <stdint.h>

#include "printf.h"
#include "regs.h"
#include "sched.h"

void do_syscall(const struct regs *rg)
{
    current->regs = *rg;
    printf("syscall %d performed\n", rg->rax);
    return;
}
