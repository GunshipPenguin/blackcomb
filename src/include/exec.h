#ifndef __EXEC_H__
#define __EXEC_H__

#include "vmm.h"
#include "ext2.h"

struct task_struct *task_from_elf(struct ext2_fs *fs, struct ext2_ino *file);

#endif /* __EXEC_H__ */
