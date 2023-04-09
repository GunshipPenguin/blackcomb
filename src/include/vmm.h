#ifndef __VMM_H
#define __VMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "defs.h"

#define PAGE_PROT_READ 1<<0
#define PAGE_PROT_WRITE 1<<1

struct vm_area {
    uint64_t start;
    uint64_t pages;

    bool user;
    uint8_t prot;

    struct vm_area *next;
    struct vm_area *prev;
};

struct mm {
    uint64_t brk;
    uint64_t p4; /* _physical_ address of the single p4 entry */

    struct vm_area *vm_areas;
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

void anon_mmap_user(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot);
void anon_mmap_kernel(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot);

void *sbrk(struct mm *mm, intptr_t increment);

void mm_add_kernel_mappings(struct mm *mm);

void mm_copy_from_mm(struct mm *dst, struct mm *src, uint64_t start, uint64_t pages);
void mm_copy_from_buf(struct mm *dst, void *src, uint64_t start, size_t len);
void mm_dupe(struct mm *old, struct mm *new);

#endif /* __VMM_H */
