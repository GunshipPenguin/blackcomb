#ifndef __EXT2_H__
#define __EXT2_H__

#include <stdint.h>

struct ext2_fs {
    uint32_t block_size;
};

void ext2_read_file(const char *path, char *buf, uint64_t off);

#endif
