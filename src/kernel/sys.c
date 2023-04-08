#include <stdint.h>

#include "printf.h"
#include "regs.h"
#include "sched.h"
#include "sys.h"
#include "util.h"

void sys_read(int fd, const void *buf, size_t count)
{
    panic("sys_read not implemented");
}

void sys_write(int fd, const void *buf, size_t count)
{
    if (fd != 1)
        panic("only writes to fd 1 (stdout) are supported");

    printf("%.*s", count, buf);
}

void sys_wait(int *wstatus)
{
    panic("sys_wait not implemented");
}

void sys_fork(void)
{
    sched_fork(current);
}

void sys_exec(const char *pathname)
{
    panic("sys_exec not implemented");
}

void sys_exit(int status)
{
    panic("sys_exit not implemented");
}

void do_syscall(const struct regs *regs)
{
    current->regs = *regs;
    printf("syscall %d performed\n", regs->rax);

    /*
     * syscall args are passed as specified in the sysV ABI: %rdi, %rsi, %rdx,
     * %r10, %r8
     */

    switch (regs->rax) {
    case SYS_READ:
        sys_read((int)regs->rdi, (void *)regs->rsi, (size_t)regs->rdx);
        break;
    case SYS_WRITE:
        sys_write((int)regs->rdi, (void *)regs->rsi, (size_t)regs->rdx);
        break;
    case SYS_WAIT:
        sys_wait((int *)regs->rdi);
        break;
    case SYS_EXEC:
        sys_exec((const char *)regs->rdi);
        break;
    case SYS_FORK:
        sys_fork();
        break;
    case SYS_EXIT:
        sys_exit((int)regs->rdi);
        break;
    default:
        panic("syscall %d not implemented\n", regs->rax);
    }

    return;
}
