#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "kmalloc.h"
#include "string.h"
#include "gdt.h"

uint64_t __kernelstack;

struct task_struct *current;

struct task_struct *tasklist_end;
struct task_struct *tasklist_start;

void schedule()
{
    struct task_struct *t = tasklist_start;
    tasklist_end->next = t;
    tasklist_end = t;
    tasklist_start = tasklist_start->next;
}

void switch_to(struct task_struct *t)
{
    __kernelstack = (uint64_t)t->kernel_stack;
    __tss.rsp0 = (uint64_t) t->kernel_stack;
    struct regs *r = &t->regs;

    asm volatile (
            "pushq %0\n"
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
            "jmp enter_usermode"
            :
            : "rm"(r->rsp), "rm"(t->mm.p4), "rm"(r->rip),
              "rm"(r->rax), "rm"(r->rbx), "rm"(r->rcx),
              "rm"(r->rdx), "rm"(r->rdi), "rm"(r->rsi),
              "rm"(r->rbp), "rm"(r->r8), "rm"(r->r9),
              "rm"(r->r10), "rm"(r->r11), "rm"(r->r12),
              "rm"(r->r13), "rm"(r->r14), "rm"(r->r15)
            : "rsp");

}

void start_init(struct ext2_fs *fs)
{
    struct ext2_ino *in;
    ext2_namei(fs, &in, "/init");

    struct task_struct *init = task_from_elf(fs, in);
    tasklist_start = init;
    tasklist_end = init;
    init->regs.r15 = 1;
    /*
    init->regs.r14 = 2;
    init->regs.r13 = 3;
    init->regs.r12 = 4;
    init->regs.r11 = 5;
    init->regs.r10 = 6;
    init->regs.r9 = 7;
    init->regs.r8 = 8;

    init->regs.rbp = 9;
    init->regs.rsi = 10;
    init->regs.rdi = 11;
    init->regs.rdx = 12;
    init->regs.rcx = 13;
    init->regs.rbx = 14;
    init->regs.rax = 15;
    init->regs.rip = 17;
    init->regs.rsp = 17;
    */

    switch_to(init);
}
