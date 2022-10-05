#ifndef __ARENA_H
#define __ARENA_H

#include "mm.h"

#include <stdint.h>
#include <stddef.h>

void *pmm_alloc_low();

void pmm_init_low(struct multiboot_tag_mmap *mmap);

#endif /* __ARENA_H */
