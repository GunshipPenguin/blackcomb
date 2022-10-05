#ifndef __MM_H
#define __MM_H

#include "multiboot2.h"

#include <stdint.h>

struct mboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};

void mm_init(void *mboot_info_start);
void *mboot_find_tag(struct mboot_info *mboot, uint32_t tag);

typedef uint64_t pte_t;

#endif
