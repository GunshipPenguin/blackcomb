#include "exec.h"
#include "ext2.h"
#include "kmalloc.h"

void map_elf(struct ext2_fs *fs, struct ext2_ino *file, struct mm_struct *mm)
{
    void *buf = ext2_read_file(fs, file);
}
