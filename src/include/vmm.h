#ifndef __VMM_H
#define __VMM_H

#include <stdint.h>

#include "pgalloc.h"

#define PGDIR_NDX(x) (((uintptr_t)x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t)x) >> 12) & 0xFFF)

void vmm_map_page(uintptr_t virt, uintptr_t phys);
void vmm_unmap_page(uintptr_t virt);

void vmm_init(struct arena_hdr *a);

void *sbrk(intptr_t increment);

#endif /* __VMM_H */
