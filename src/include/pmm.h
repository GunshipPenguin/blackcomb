#ifndef __ARENA_H
#define __ARENA_H

#include "mm.h"

#include <stdint.h>
#include <stddef.h>

void pmm_set_mmap(struct multiboot_tag_mmap *mmap)

void *pmm_alloc_low();
void *pmm_alloc_high();

void pmm_init_low();
void pmm_init_high();

#endif /* __ARENA_H */
