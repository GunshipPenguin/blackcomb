#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "fork.h"
#include "gdt.h"
#include "kmalloc.h"
#include "string.h"
#include "tty.h"
#include "util.h"
#include "vmm.h"

#define PREEMPT_TICK_COUNT 10

extern uint64_t __init_stack_top;
uint64_t __current_task_stack;

struct task_struct *current;
struct task_struct *proc_root;

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

void start_init()
{
    struct task_struct *init = create_init();

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

int sched_fork(struct task_struct *t)
{
    struct task_struct *new = do_fork(t);

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
