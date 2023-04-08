#ifndef __VMM_H
#define __VMM_H

#include <stdint.h>

#include "defs.h"

struct mm {
    uint64_t brk;
    uint64_t p4; /* _physical_ address of the single p4 entry */
};

extern struct mm kernel_mm;

#define USER_STACK_BASE 0x7ffffffde000
#define USER_STACK_PAGES 32
#define USER_STACK_START (USER_STACK_BASE + (USER_STACK_PAGES * PAGE_SIZE))

#define KERNEL_STACK_BASE 0xffffff0000000000
#define KERNEL_STACK_PAGES 16
#define KERNEL_STACK_START (KERNEL_STACK_BASE + (KERNEL_STACK_PAGES * PAGE_SIZE))


#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P_STATIC(x) (((uint64_t)(x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000
#define P_TO_V(type, x) ((type *)(PHYS_MAPPING_START + ((uint64_t)x)));

void switch_cr3(uint64_t addr);

void anon_mmap(struct mm *mm, uint64_t start, uint64_t pages);

void vmm_map_page(struct mm *mm, uintptr_t virt, uintptr_t phys);
void vmm_unmap_page(struct mm *mm, uintptr_t virt);

uintptr_t vmm_get_phys(struct mm *mm, uintptr_t virt);

void *sbrk(struct mm *mm, intptr_t increment);

void mm_add_kernel_mappings(struct mm *mm);

#endif /* __VMM_H */
