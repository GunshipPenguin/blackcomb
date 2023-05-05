#include <stdint.h>

#include "tty.h"
#include "printf.h"
#include "regs.h"
#include "sched.h"
#include "sys.h"
#include "util.h"

int sys_read(int fd, const void *buf, size_t count)
{
    if (fd != 0)
        panic("sys_read not implemented for fd != 0");

    wait_tty(buf, count);
}

int sys_write(int fd, const void *buf, size_t count)
{
    if (fd != 1)
        panic("only writes to fd 1 (stdout) are supported");

    printf("%.*s", count, buf);
}

int sys_wait(int *wstatus)
{
    panic("sys_wait not implemented");
}

int sys_fork(void)
{
    sched_fork(current);
}

int sys_exec(const char *pathname)
{
    sched_exec(pathname, current);
}

int sys_exit(int status)
{
    panic("sys_exit not implemented");
}

void do_syscall(struct regs *regs)
{
    current->regs = regs;
//    printf("syscall %d performed\n", regs->rax);

    /*
     * syscall args are passed as specified in the sysV ABI: %rdi, %rsi, %rdx,
     * %r10, %r8
     */

    switch (regs->rax) {
    case SYS_READ:
        regs->rax = sys_read((int)regs->rdi, (void *)regs->rsi, (size_t)regs->rdx);
        break;
    case SYS_WRITE:
        regs->rax = sys_write((int)regs->rdi, (void *)regs->rsi, (size_t)regs->rdx);
        break;
    case SYS_WAIT:
        regs->rax = sys_wait((int *)regs->rdi);
        break;
    case SYS_EXEC:
        regs->rax = sys_exec((const char *)regs->rdi);
        break;
    case SYS_FORK:
        regs->rax = sys_fork();
        break;
    case SYS_EXIT:
        regs->rax = sys_exit((int)regs->rdi);
        break;
    default:
        panic("syscall %d not implemented\n", regs->rax);
    }

    return;
}
