#ifndef __EXEC_H__
#define __EXEC_H__

#include "mm.h"
#include "ext2.h"

void map_elf(struct ext2_fs *fs, struct ext2_ino *file, struct mm_struct *mm);

#endif /* __EXEC_H__ */
