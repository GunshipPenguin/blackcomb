#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "gdt.h"
#include "kmalloc.h"
#include "string.h"
#include "vmm.h"
#include "util.h"

#define PREEMPT_TICK_COUNT 10

extern uint64_t __init_stack_top;
uint64_t __current_task_stack;

// 2^32 pids ought to be enough for anyone ;)
// TODO: Fix this to not collide when it wraps
uint32_t next_pid;

struct task_struct *current;
struct task_struct *proc_root;

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

void switch_to(struct task_struct *t)
{
    struct task_struct *current_old = current;
    current = t;

    if (current_old->pid == current->pid)
        panic("kernel bug: switch to same process");

    __current_task_stack = t->sp;
    __tss.rsp0 = t->sp;
    switch_cr3(t->mm->p4);

    extern void switch_to_asm(uint64_t new_rsp, uint64_t * put_old_rsp);
    switch_to_asm(t->sp, &current_old->sp);
}

void tree_insert_proc(struct task_struct *parent, struct task_struct *t)
{
    t->parent = parent;
    if (parent->children)
        t->siblings = parent->children->siblings;
    else
        t->siblings = NULL;

    parent->children = t;
}

void sched_rr_insert_proc(struct task_struct *t)
{
    t->next = current->next;
    t->prev = current;

    current->next->prev = t;
    current->next = t;
}

void sched_rr_remove_proc(struct task_struct *t)
{
    t->prev->next = t->next;
    t->next->prev = t->prev;

    t->next = NULL;
    t->prev = NULL;
}

void init_task_stack(struct task_struct *t, struct regs *regs)
{
    t->stack_bottom = kcalloc(1, KERNEL_STACK_SIZE);
    void *stack_top = (((char *)t->stack_bottom) + KERNEL_STACK_SIZE);

    t->sp = (uint64_t)stack_top - sizeof(struct fork_frame);
    struct fork_frame *ff = (struct fork_frame *)t->sp;

    ff->itf.rbp = 0;
    extern void pop_regs_and_sysret();
    ff->itf.ret_addr = (uint64_t)&pop_regs_and_sysret;

    ff->regs = *regs;
}

void start_init()
{
    struct ext2_ino *in;
    ext2_namei(rootfs, &in, "/init");
    struct task_struct *init = kcalloc(1, sizeof(struct task_struct));
    init->regs = kcalloc(1, sizeof(struct regs));

    exec_elf(init, rootfs, in);
    init_task_stack(init, init->regs);

    init->pid = 1;
    init->alive_ticks = 0;

    init->next = init;
    init->prev = init;
    init->state = TASK_RUNNING;

    next_pid = 1000;

    proc_root = init;
    current = init;
    __current_task_stack = init->sp;
    __tss.rsp0 = init->sp;
    switch_cr3(init->mm->p4);

    uint64_t put_old; /* dummy space */
    extern void switch_to_asm(uint64_t new_rsp, uint64_t * put_old_rsp);
    switch_to_asm(init->sp, &put_old);
}

int sched_exec(const char *path, struct task_struct *t)
{
    mm_free(t->mm);
    t->mm = mm_new();

    printf("%d doing exec\n", t->pid);
    struct ext2_ino *in;
    ext2_namei(rootfs, &in, path);
    exec_elf(t, rootfs, in);

    switch_cr3(t->mm->p4);

    return 0;
}

int sched_wait(struct task_struct *t, int *wstatus)
{
    panic("unimplemented");
}

int sched_exit(struct task_struct *t)
{
    panic("unimplemented");
}

int sched_fork(struct task_struct *t)
{
    struct task_struct *new = kcalloc(1, sizeof(struct task_struct));
    printf("%d doing fork\n", t->pid);

    new->regs = kcalloc(1, sizeof(struct regs));
    new->state = TASK_RUNNING;

    new->mm = mm_dupe(t->mm);
    new->pid = next_pid++;

    struct regs regs = *t->regs;
    regs.rax = 0;

    init_task_stack(new, &regs);
    sched_rr_insert_proc(new);
    tree_insert_proc(t, new);

    return new->pid;
}

void schedule()
{
    current->alive_ticks = 0;
    if (current == current->next)
        return;

    switch_to(current->next);
}

void schedule_sleep()
{
    current->alive_ticks = 0;
    struct task_struct *next = current->next;
    sched_rr_remove_proc(current);
    switch_to(next);
}

void sched_maybe_preempt()
{
    if (current->alive_ticks++ < PREEMPT_TICK_COUNT)
        return;

    schedule();
}
