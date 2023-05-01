#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "gdt.h"
#include "kmalloc.h"
#include "string.h"
#include "vmm.h"

#define PREEMPT_TICK_COUNT 10

uint64_t __kernelstack;

// 2^32 pids ought to be enough for anyone ;)
// TODO: Fix this to not collide when it wraps
uint32_t next_pid;

struct task_struct *current;

void switch_to(struct task_struct *t)
{
    current = t;

    __kernelstack = KERNEL_STACK_START;
    __tss.rsp0 = KERNEL_STACK_START;
}

__attribute__((noreturn)) void enter_usermode()
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
                 : "rm"(r->rsp), "rm"(current->mm->p4), "rm"(r->rip), "rm"(r->rax), "rm"(r->rbx),
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

void start_init()
{
    struct ext2_ino *in;
    ext2_namei(rootfs, &in, "/init");
    struct task_struct *init = kcalloc(1, sizeof(struct task_struct));
    exec_elf(init, rootfs, in);

    init->pid = 1;
    init->alive_ticks = 0;

    init->next = init;
    init->prev = init;

    next_pid = 1000;

    switch_to(init);
    enter_usermode();
}

int sched_exec(const char *path, struct task_struct *t)
{
    mm_free(t->mm);
    t->mm = mm_new();

    struct ext2_ino *in;
    ext2_namei(rootfs, &in, path);
    exec_elf(t, rootfs, in);

    enter_usermode();
}

int sched_fork(struct task_struct *t)
{
    struct task_struct *new = kcalloc(1, sizeof(struct task_struct));

    new->mm = mm_dupe(t->mm);
    new->regs = t->regs;
    new->pid = next_pid++;
    new->regs.rax = 0;

    insert_proc(new);

    return new->pid;
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
