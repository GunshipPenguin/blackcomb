#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "gdt.h"
#include "kmalloc.h"
#include "string.h"
#include "util.h"
#include "vmm.h"

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

    __current_task_stack = t->stack_top;
    __tss.rsp0 = t->sp;
    switch_cr3(t->mm->p4);

    extern void switch_to_asm(uint64_t new_rsp, uint64_t * put_old_rsp);
    switch_to_asm(t->sp, &current_old->sp);
}

void tree_insert_proc(struct task_struct *parent, struct task_struct *t)
{
    t->parent = parent;
    if (parent->children)
        t->sibling = parent->children->sibling;
    else
        t->sibling = NULL;

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

void start_init()
{
    struct task_struct *init = kcalloc(1, sizeof(struct task_struct));
    init_task_stack(init);

    exec_elf(init, "/init");

    init->pid = 1;
    init->alive_ticks = 0;

    init->next = init;
    init->prev = init;
    init->state = TASK_RUNNING;

    next_pid = 1000;

    proc_root = init;
    current = init;
    __current_task_stack = init->stack_top;
    __tss.rsp0 = init->sp;
    switch_cr3(init->mm->p4);

    uint64_t put_old; /* dummy space */
    extern void switch_to_asm(uint64_t new_rsp, uint64_t * put_old_rsp);
    switch_to_asm(init->sp, &put_old);
}

int sched_exec(const char *path, struct task_struct *t)
{
    struct mm *mm_old = t->mm;
    exec_elf(t, path);

    /* Cannot do mm_free until after exec_elf as pathname is in old address space. */
    switch_cr3(kernel_mm.p4);
    mm_free(mm_old);
    switch_cr3(t->mm->p4);

    return 0;
}

void task_free(struct task_struct *t)
{
    mm_free(t->mm);
    free((void *)t->stack_bottom);
    free(t);
}

int sched_wait(struct task_struct *t, int *wstatus)
{
    struct task_struct *child;

retry:
    child = t->children;
    while (child) {
        if (child->state == TASK_ZOMBIE)
            break;

        child = child->sibling;
    }

    if (!child) {
        /* No zombie children, sleep until a child exits */
        current->state = TASK_WAITING;
        schedule_sleep();
        goto retry;
    }

    *wstatus = t->exit_status;
    int ret = child->pid;
    task_free(child);
    return ret;
}

int sched_exit(struct task_struct *t, int status)
{
    t->state = TASK_ZOMBIE;
    t->exit_status = status;

    if (t->parent->state == TASK_WAITING) {
        sched_rr_insert_proc(t->parent);
        t->parent->state = TASK_RUNNING;
    }

    schedule_sleep();
    return 0; /* Never hit */
}

int sched_fork(struct task_struct *t)
{
    struct task_struct *new = kcalloc(1, sizeof(struct task_struct));

    new->state = TASK_RUNNING;
    new->mm = mm_dupe(t->mm);
    new->pid = next_pid++;

    init_task_stack(new);

    /* new->regs now initialized and pointing to the fork frame */
    *new->regs = *t->regs;
    new->regs->rax = 0;

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
