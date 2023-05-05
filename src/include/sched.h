#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdbool.h>
#include <stdint.h>

#include "vmm.h"
#include "regs.h"

enum task_state {
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE,
};

struct task_struct {
    uint32_t pid;
    enum task_state state;

    struct regs *regs;

    struct mm *mm;

    // Return value from kcalloc, start sp is this + KERNEL_STACK_SIZE
    void *stack_bottom;
    uint64_t sp;

    struct task_struct *next;
    struct task_struct *prev;

    struct task_struct *parent;
    struct task_struct *children;
    struct task_struct *siblings;

    uint64_t alive_ticks;
};

extern struct task_struct *current;

void start_init();
void switch_to(struct task_struct *t);

void sched_rr_insert_proc(struct task_struct *t);
void sched_rr_remove_proc(struct task_struct *t);

int sched_fork(struct task_struct *t);
int sched_exec(const char *path, struct task_struct *t);
int sched_wait(struct task_struct *t, int *wstatus);
int sched_exit(struct task_struct *t);

void sched_maybe_preempt();

void schedule();
void schedule_sleep();

#endif /* __SCHED_H__ */
