#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdbool.h>
#include <stdint.h>

#include "vmm.h"
#include "regs.h"

struct task_struct {
    uint32_t pid;

    struct regs regs;
    struct mm *mm;

    struct task_struct *next;
    struct task_struct *prev;

    uint64_t alive_ticks;
};

extern struct task_struct *current;

void start_init();
void switch_to(struct task_struct *t);

bool sched_maybe_preempt();

int sched_fork(struct task_struct *t);
int sched_exec(const char *path, struct task_struct *t);

#endif /* __SCHED_H__ */
