#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdbool.h>
#include <stdint.h>

#include "vmm.h"
#include "regs.h"

#define TASK_FILE_MAX 16

enum task_state {
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_WAITING,
    TASK_ZOMBIE,
};

struct task_struct {
    uint32_t pid;
    enum task_state state;
    int exit_status;

    struct regs *regs;

    struct mm *mm;

    /* Return value from kcalloc */
    uint64_t stack_bottom;

    /* stack_bottom + KERNEL_STACK_SIZE */
    uint64_t stack_top;

    uint64_t sp;

    /* scheduling linked list pointers */
    struct task_struct *next;
    struct task_struct *prev;

    /*
     * Process tree linked list pointers. task->sibling forms a singly linked
     * list among all children of the same parent.
     */
    struct task_struct *parent;
    struct task_struct *children;
    struct task_struct *sibling;

    struct file *fdtab[TASK_FILE_MAX];

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
int sched_exit(struct task_struct *t, int status);

void sched_maybe_preempt();

void schedule();
void schedule_sleep();

#endif /* __SCHED_H__ */
