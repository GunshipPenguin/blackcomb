#ifndef __ARENA_H
#define __ARENA_H

#include <stdint.h>
#include <stddef.h>

uint64_t pmm_alloc();
void pmm_free(uint64_t phys);

void pmm_init(void *mboot_info_start);

#endif /* __ARENA_H */
