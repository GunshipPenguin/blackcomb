#ifndef __EXEC_H__
#define __EXEC_H__

#include "ext2.h"
#include "sched.h"
#include "vmm.h"

void exec_elf(struct task_struct *task, struct ext2_fs *fs, struct ext2_ino *file);

#endif /* __EXEC_H__ */
