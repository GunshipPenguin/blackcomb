#ifndef __ARENA_H
#define __ARENA_H

#include "mm.h"

#include <stdint.h>
#include <stddef.h>

uint64_t pmm_alloc();
void pmm_free(uint64_t phys);

void pmm_init();

#endif /* __ARENA_H */
