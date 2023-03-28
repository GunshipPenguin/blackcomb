#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdbool.h>
#include <stdint.h>

#include "ext2.h"
#include "vmm.h"
#include "regs.h"

struct task_struct {
    uint32_t pid;

    void *kernel_stack_start;
    void *kernel_stack_end;

    struct regs regs;
    struct mm mm;

    struct task_struct *next;
    struct task_struct *prev;

    uint64_t alive_ticks;
};

extern struct task_struct *current;

void start_init(struct ext2_fs *fs);
void switch_to(struct task_struct *t);

bool sched_maybe_preempt();

#endif /* __SCHED_H__ */
