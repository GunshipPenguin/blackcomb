#ifndef __VMM_H
#define __VMM_H

#include <stdint.h>

struct mm {
    uint64_t brk;
    uint64_t p4; /* _physical_ address of the single p4 entry */
};

#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P_STATIC(x) (((uint64_t)(x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000
#define P_TO_V(type, x) ((type *)(PHYS_MAPPING_START + ((uint64_t)x)));

void switch_cr3(uint64_t addr);
void vmm_map_page(struct mm *mm, uintptr_t virt, uintptr_t phys);
void vmm_unmap_page(struct mm *mm, uintptr_t virt);
uintptr_t vmm_get_phys(struct mm *mm, uintptr_t virt);
void init_kernel_mm(struct mm *mm);
void *sbrk(struct mm *mm, intptr_t increment);

#endif /* __VMM_H */
