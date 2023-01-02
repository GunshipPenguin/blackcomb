#ifndef __ARENA_H
#define __ARENA_H

#include "mm.h"

#include <stdint.h>
#include <stddef.h>

void pmm_set_mmap(struct multiboot_tag_mmap *mmap);

uint64_t pmm_alloc_low();
uint64_t pmm_alloc_high();

void pmm_init_low();
void pmm_init_high();

void pmm_free(uint64_t phys);

#endif /* __ARENA_H */
