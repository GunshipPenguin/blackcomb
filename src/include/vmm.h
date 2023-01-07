#ifndef __VMM_H
#define __VMM_H

#include <stdint.h>

#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P_STATIC(x) (((uint64_t)(x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000
#define P_TO_V(type, x) ((type *)(PHYS_MAPPING_START + ((uint64_t)x)));

void vmm_map_page(uintptr_t virt, uintptr_t phys);
void vmm_unmap_page(uintptr_t virt);
uintptr_t vmm_get_phys(uintptr_t virt);

void vmm_init();

void *sbrk(intptr_t increment);

#endif /* __VMM_H */
