#include "fork.h"
#include "exec.h"
#include "file.h"
#include "kmalloc.h"
#include "sched.h"
#include "string.h"
#include "tty.h"

#include <stdint.h>

// 2^32 pids ought to be enough for anyone ;)
// TODO: Fix this to not collide when it wraps
uint32_t next_pid = 1000;

struct inactive_task_frame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rbx;

    uint64_t rbp;
    uint64_t ret_addr;
};

struct fork_frame {
    struct inactive_task_frame itf;
    struct regs regs;
};

void init_task_stack(struct task_struct *t)
{
    t->stack_bottom = (uint64_t)kcalloc(1, KERNEL_STACK_SIZE);
    t->stack_top = t->stack_bottom + KERNEL_STACK_SIZE;

    t->sp = t->stack_top - sizeof(struct fork_frame);
    struct fork_frame *ff = (struct fork_frame *)t->sp;

    ff->itf.rbp = 0;
    extern void pop_regs_and_sysret();
    ff->itf.ret_addr = (uint64_t)&pop_regs_and_sysret;

    t->regs = &ff->regs;
}

struct task_struct *do_fork(struct task_struct *t)
{
    struct task_struct *new = kcalloc(1, sizeof(struct task_struct));

    new->state = TASK_RUNNING;
    new->mm = mm_dupe(t->mm);
    new->pid = next_pid++;

    for (int i = 0; i < TASK_FILE_MAX; i++) {
        if (t->fdtab[i]) {
            t->fdtab[i]->refcnt++;
            new->fdtab[i] = t->fdtab[i];
        }
    }

    init_task_stack(new);

    /* new->regs now initialized and pointing to the fork frame */
    *new->regs = *t->regs;
    new->regs->rax = 0;

    return new;
}

struct task_struct *create_init()
{
    struct task_struct *init = kcalloc(1, sizeof(struct task_struct));
    init_task_stack(init);

    exec_elf(init, "/init");

    init->pid = 1;
    init->alive_ticks = 0;

    init->next = init;
    init->prev = init;
    init->state = TASK_RUNNING;
    tty_install_stdin_stdout(init);

    return init;
}
