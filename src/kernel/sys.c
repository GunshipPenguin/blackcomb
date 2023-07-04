#include <stdint.h>

#include "exit.h"
#include "file.h"
#include "printf.h"
#include "regs.h"
#include "sched.h"
#include "sys.h"
#include "tty.h"
#include "util.h"

int64_t sys_read(int fd, void *buf, size_t count)
{
    struct file *f = current->fdtab[fd];
    if (!f)
        panic("fd %d does not exist", fd);

    if (!f->ops->read)
        panic("fd %d ENOSYS", fd);

    return f->ops->read(f, buf, count);
}

int64_t sys_write(int fd, const void *buf, size_t count)
{
    struct file *f = current->fdtab[fd];
    if (!f)
        panic("fd %d does not exist", fd);

    if (!f->ops->write)
        panic("fd %d ENOSYS", fd);

    return f->ops->write(f, buf, count);
}

int sys_wait(int *wstatus)
{
    return do_wait(current, wstatus);
}

int sys_open(char *path)
{
    return do_open(path);
}

int sys_close(int fd)
{
    return do_close(fd);
}

int sys_fork(void)
{
    return sched_fork(current);
}

int sys_exec(const char *pathname)
{
    return sched_exec(pathname, current);
}

int sys_exit(int status)
{
    return do_exit(current, status);
}

int sys_getdents(int fd, struct dirent *buf, size_t size)
{
    struct file *f = current->fdtab[fd];
    if (!f)
        panic("fd %d does not exist", fd);

    if (!f->ops->getdents)
        panic("fd %d ENOSYS", fd);

    return f->ops->getdents(f, buf, size);
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
    case SYS_OPEN:
        regs->rax = sys_open((void *)regs->rdi);
        break;
    case SYS_CLOSE:
        regs->rax = sys_close((int)regs->rdi);
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
    case SYS_GETDENTS:
        regs->rax = sys_getdents((int)regs->rdi, (struct dirent *)regs->rsi, (size_t)regs->rdx);
        break;
    default:
        panic("syscall %d not implemented\n", regs->rax);
    }

    return;
}
