#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "gdt.h"
#include "kmalloc.h"
#include "string.h"
#include "vmm.h"

#define PREEMPT_TICK_COUNT 10

uint64_t __kernelstack;

struct task_struct *current;

void switch_to(struct task_struct *t)
{
    current = t;

    __kernelstack = KERNEL_STACK_START;
    __tss.rsp0 = KERNEL_STACK_START;
}

void enter_usermode()
{
    struct regs *r = &current->regs;
    asm volatile("pushq %0\n"
                 "pushq %1\n"
                 "pushq %2\n"
                 "pushq %3\n"
                 "pushq %4\n"
                 "pushq %5\n"
                 "pushq %6\n"
                 "pushq %7\n"
                 "pushq %8\n"
                 "pushq %9\n"
                 "pushq %10\n"
                 "pushq %11\n"
                 "pushq %12\n"
                 "pushq %13\n"
                 "pushq %14\n"
                 "pushq %15\n"
                 "pushq %16\n"
                 "pushq %17\n"
                 "jmp usermode_tramp"
                 :
                 : "rm"(r->rsp), "rm"(current->mm.p4), "rm"(r->rip), "rm"(r->rax), "rm"(r->rbx),
                   "rm"(r->rcx), "rm"(r->rdx), "rm"(r->rdi), "rm"(r->rsi), "rm"(r->rbp),
                   "rm"(r->r8), "rm"(r->r9), "rm"(r->r10), "rm"(r->r11), "rm"(r->r12), "rm"(r->r13),
                   "rm"(r->r14), "rm"(r->r15)
                 : "rsp");
}

void insert_proc(struct task_struct *t)
{
    t->next = current->next;
    t->prev = current;

    current->next->prev = t;
    current->next = t;
}

void start_init(struct ext2_fs *fs)
{
    struct ext2_ino *in;
    ext2_namei(fs, &in, "/init");
    struct task_struct *init = task_from_elf(fs, in);

    init->pid = 1;
    init->alive_ticks = 0;

    init->next = init;
    init->prev = init;

    switch_to(init);
    enter_usermode();
}

void sched_fork(struct task_struct *t)
{
    struct task_struct *new = kcalloc(1, sizeof(struct task_struct));

    mm_dupe(&t->mm, &new->mm);
    new->regs = t->regs;

    insert_proc(new);
}

bool sched_maybe_preempt()
{
    if (current->alive_ticks < PREEMPT_TICK_COUNT)
        goto out;

    printf("preempting process %d\n", current->pid);
    current->alive_ticks = 0;

    switch_to(current->next);
    printf("new current pid is %d\n", current->pid);
    return true;

out:
    current->alive_ticks++;
    return false;
}
