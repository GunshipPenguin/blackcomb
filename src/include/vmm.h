#ifndef __VMM_H
#define __VMM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "defs.h"

#define PAGE_PROT_READ 1<<0
#define PAGE_PROT_WRITE 1<<1

enum vm_area_type {
    VM_AREA_REG,
    VM_AREA_KERNELTEXT,
    VM_AREA_KERNELHEAP,
    VM_AREA_PHYSMEM,
};

struct vm_area {
    int refcnt;
    enum vm_area_type type;

    uint64_t start;
    uint64_t pages;

    bool user;
    uint8_t prot;
};

struct vm_area_li {
    struct vm_area *vma;

    struct vm_area_li *next;
    struct vm_area_li *prev;
};

struct mm {
    uint64_t p4; /* _physical_ address of the single p4 entry */
    struct vm_area_li *vm_areas;
};

extern struct mm kernel_mm;

#define USER_STACK_BASE 0x7ffffffde000
#define USER_STACK_PAGES 32
#define USER_STACK_START (USER_STACK_BASE + (USER_STACK_PAGES * PAGE_SIZE))

#define KERNEL_STACK_SIZE 8192

#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P_STATIC(x) (((uint64_t)(x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000
#define P_TO_V(type, x) ((type *)(PHYS_MAPPING_START + ((uint64_t)x)));

void kernel_mm_init(struct mm *mm);

struct mm *mm_dupe(struct mm *mm);
struct mm *mm_new();
void mm_init();

void mm_free(struct mm *old);

void anon_mmap_user(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot);
void anon_mmap_kernel(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot);
void mm_copy_from_buf(struct mm *dst, void *src, uint64_t start, size_t len);

void *sbrk(intptr_t increment);

void switch_cr3(uint64_t addr);

#endif /* __VMM_H */
