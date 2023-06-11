#include <stdint.h>

#include "printf.h"
#include "regs.h"
#include "sched.h"
#include "sys.h"
#include "tty.h"
#include "util.h"

int64_t sys_read(int fd, void *buf, size_t count)
{
    switch (fd) {
    case 0:
        return tty_read(buf, count);
    default:
        panic("sys_read not implemented for fd != 0");
    }
}

int64_t sys_write(int fd, const void *buf, size_t count)
{
    switch (fd) {
    case 1:
        printf("%.*s", count, buf);
        return count;
    default:
        panic("only writes to fd 1 (stdout) are supported");
    }
}

int sys_wait(int *wstatus)
{
    return sched_wait(current, wstatus);
}

void sys_fork(void)
{
    sched_fork(current);
}

int sys_exec(const char *pathname)
{
    return sched_exec(pathname, current);
}

int sys_exit(int status)
{
    panic("sys_exit not implemented");
}

void do_syscall(struct regs *regs)
{
    current->regs = regs;

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
        /*
         * fork is kinda funky because we need to set rax in the parent and
         * child, let the code in sched.h handle that.
         */
        sys_fork();
        break;
    case SYS_EXIT:
        regs->rax = sys_exit((int)regs->rdi);
        break;
    default:
        panic("syscall %d not implemented\n", regs->rax);
    }

    return;
}
