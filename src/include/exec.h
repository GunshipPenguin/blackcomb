#ifndef __EXEC_H__
#define __EXEC_H__

#include "sched.h"
#include "vmm.h"

void exec_elf(struct task_struct *task, const char *path);

#endif /* __EXEC_H__ */
