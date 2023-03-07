#ifndef __SCHED_H__
#define __SCHED_H__

#include <stdint.h>

#include "ext2.h"
#include "vmm.h"
#include "regs.h"

struct task_struct {
    void *kernel_stack;
    struct regs regs;
    struct mm mm;

    struct task_struct *next;
    struct task_struct *prev;
};

extern struct task_struct *current;

void start_init(struct ext2_fs *fs);

#endif /* __SCHED_H__ */
