#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "kmalloc.h"
#include "string.h"

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

void start_init(struct ext2_fs *fs)
{
    struct ext2_ino *in;
    ext2_namei(fs, &in, "/init");

    struct task_struct *t = task_from_elf(fs, in);
    tasklist_start = t;
    tasklist_end = t;
    t->regs.r15 = 5;
    initial_enter_usermode(t->regs.rip, t->regs.rsp, t->mm.p4);
}
