#include "vmm.h"

#include "defs.h"
#include "multiboot2.h"
#include "string.h"
#include "kmalloc.h"
#include "pmm.h"
#include "util.h"

#define P4_NDX(x) (((x) >> 39) & 0x1FF)
#define P3_NDX(x) (((x) >> 30) & 0x1FF)
#define P2_NDX(x) (((x) >> 21) & 0x1FF)
#define P1_NDX(x) (((x) >> 12) & 0x1FF)

#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_SIZE_FLAG (1 << 7)

#define KERNEL_BRK_START 0xFFFFC90000000000

struct mm kernel_mm;

void switch_cr3(uint64_t addr)
{
    asm volatile("mov %0, %%cr3" : : "r"(addr));
}

void vmm_map_range(struct mm *mm, uint64_t virt, uint64_t phys, uint64_t npages)
{
    if ((virt & PAGE_MASK) || (phys & PAGE_MASK))
        panic("mapping is not paged aligned");

    for (int pg = 0; pg < npages; pg++) {
        uint64_t off = pg * PAGE_SIZE;
        vmm_map_page(mm, virt + off, phys + off);
    }
}

void vmm_unmap_range(struct mm *mm, uint64_t virt, uint64_t npages)
{
    if (virt & PAGE_MASK)
        panic("mapping is not paged aligned");

    for (int pg = 0; pg < npages; pg++) {
        vmm_unmap_page(mm, virt + (pg * PAGE_SIZE));
    }
}

uint64_t p4_get_entry(struct mm *mm, int i)
{
    uint64_t *p4_base_addr = P_TO_V(uint64_t, mm->p4);
    return p4_base_addr[i];
}

uint64_t p3_get_entry(struct mm *mm, int p4_i, int i)
{
    uint64_t *p3_base_addr = P_TO_V(uint64_t, p4_get_entry(mm, p4_i) & ~PAGE_MASK);
    return p3_base_addr[i];
}

uint64_t p2_get_entry(struct mm *mm, int p4_i, int p3_i, int i)
{
    uint64_t *p2_base_addr = P_TO_V(uint64_t, p3_get_entry(mm, p4_i, p3_i) & ~PAGE_MASK);
    return p2_base_addr[i];
}

uint64_t p1_get_entry(struct mm *mm, int p4_i, int p3_i, int p2_i, int i)
{
    uint64_t *p1_base_addr = P_TO_V(uint64_t, p2_get_entry(mm, p4_i, p3_i, p2_i) & ~PAGE_MASK);
    return p1_base_addr[i];
}

void p4_set_entry(struct mm *mm, int i, uint64_t val)
{
    uint64_t *p4_base_addr = P_TO_V(uint64_t, mm->p4);
    p4_base_addr[i] = val;
}

void p3_set_entry(struct mm *mm, int p4_i, int i, uint64_t val)
{
    uint64_t *p3_base_addr = P_TO_V(uint64_t, p4_get_entry(mm, p4_i) & ~PAGE_MASK);
    p3_base_addr[i] = val;
}

void p2_set_entry(struct mm *mm, int p4_i, int p3_i, int i, uint64_t val)
{
    uint64_t *p2_base_addr = P_TO_V(uint64_t, p3_get_entry(mm, p4_i, p3_i) & ~PAGE_MASK);
    p2_base_addr[i] = val;
}

void p1_set_entry(struct mm *mm, int p4_i, int p3_i, int p2_i, int i, uint64_t val)
{
    uint64_t *p1_base_addr = P_TO_V(uint64_t, p2_get_entry(mm, p4_i, p3_i, p2_i) & ~PAGE_MASK);
    p1_base_addr[i] = val;
}

uintptr_t vmm_get_phys(struct mm *mm, uintptr_t virt)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    return p1_get_entry(mm, p4_i, p3_i, p2_i, p1_i) & ~PAGE_MASK;
}

void vmm_map_page(struct mm *mm, uintptr_t virt, uintptr_t phys)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    if (mm->p4 == 0) {
        /* No p4 allocated here, get a free physical page */
        mm->p4 = pmm_alloc();
    }

    if (p4_get_entry(mm, p4_i) == 0) {
        /* No p3 allocated here, get a free physical page */
        uintptr_t new_p3 = pmm_alloc();
        uint64_t entry = new_p3 | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        p4_set_entry(mm, p4_i, entry);
    }

    if (p3_get_entry(mm, p4_i, p3_i) == 0) {
        /* No p2 allocated here, get a free physical page */
        uintptr_t new_p2 = pmm_alloc();
        uint64_t entry = new_p2 | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        p3_set_entry(mm, p4_i, p3_i, entry);
    }

    if (p2_get_entry(mm, p4_i, p3_i, p2_i) == 0) {
        /* No p1 allocated here, get a free physical page */
        uintptr_t new_p1 = pmm_alloc();
        uint64_t entry = new_p1 | PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        p2_set_entry(mm, p4_i, p3_i, p2_i, entry);
    }

    p1_set_entry(mm, p4_i, p3_i, p2_i, p1_i, phys | PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
}

void vmm_unmap_page(struct mm *mm, uintptr_t virt)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    p1_set_entry(mm, p4_i, p3_i, p2_i, p1_i, 0);

    /* TODO: This works but is slow as hell, do something less naive */

    for (int i = 0; i < 1023; i++) {
        if (p1_get_entry(mm, p4_i, p3_i, p2_i, i) != 0)
            goto maybe_free_p2;
    }

    /* Entire P1 is 0, we can free the frame */
    pmm_free(p2_get_entry(mm, p4_i, p3_i, p2_i) & ~PAGE_MASK);
    p2_set_entry(mm, p4_i, p3_i, p2_i, 0);

maybe_free_p2:
    for (int i = 0; i < 1023; i++) {
        if (p2_get_entry(mm, p4_i, p3_i, i) != 0)
            goto maybe_free_p3;
    }

    /* Entire P2 is 0, we can free the frame */
    pmm_free(p3_get_entry(mm, p4_i, p3_i) & ~PAGE_MASK);
    p3_set_entry(mm, p4_i, p3_i, 0);

maybe_free_p3:
    for (int i = 0; i < 1023; i++) {
        if (p3_get_entry(mm, p4_i, i) != 0)
            goto out;
    }

    /* Entire P3 is 0, we can free the frame */
    pmm_free(p4_get_entry(mm, p4_i) & ~PAGE_MASK);
    p4_set_entry(mm, p4_i, 0);

out:
    return;
}

void anon_mmap(struct mm *mm, uint64_t start, uint64_t pages)
{
    if (start & PAGE_MASK)
        panic("mmap is not page aligned");

    for (int i = 0; i < pages; i++) {
        uint64_t off = i * PAGE_SIZE;
        uint64_t frame = pmm_alloc();
        vmm_map_page(mm, start + off, frame);
    }

    struct vm_area *area = kmalloc(sizeof(struct vm_area));
    area->start = start;
    area->pages = pages;
    area->prev = NULL;
    area->next = mm->vm_areas;

    if (mm->vm_areas)
        mm->vm_areas->prev = area;

    mm->vm_areas = area;
}

void mm_copy_from_mm(struct mm *dst, struct mm *src, uint64_t start, uint64_t pages)
{
    if (start & PAGE_MASK)
        panic("copy_physmem: start is not page aligned");

    for (uint64_t pg = 0; pg < pages; pg++) {
        uint64_t p4 = P4_NDX(start + pg);
        uint64_t p3 = P3_NDX(start + pg);
        uint64_t p2 = P2_NDX(start + pg);
        uint64_t p1 = P1_NDX(start + pg);

        uint64_t *src_data = P_TO_V(uint64_t, p1_get_entry(src, p4, p3, p2, p1) & ~PAGE_MASK);
        uint64_t *dst_data = P_TO_V(uint64_t, p1_get_entry(dst, p4, p3, p2, p1) & ~PAGE_MASK);
        memcpy(src_data, dst_data, PAGE_SIZE);
    }
}

void mm_copy_from_buf(struct mm *dst, void *src, uint64_t start, size_t len)
{
    if (start & PAGE_MASK)
        panic("copy_physmem: start is not page aligned");

    size_t rem = len;
    uint64_t pg = 0;
    while (rem > 0) {
        uint64_t p4 = P4_NDX(start + pg);
        uint64_t p3 = P3_NDX(start + pg);
        uint64_t p2 = P2_NDX(start + pg);
        uint64_t p1 = P1_NDX(start + pg);

        uint64_t *dst_data = P_TO_V(uint64_t, p1_get_entry(dst, p4, p3, p2, p1) & ~PAGE_MASK);

        size_t bytes = len > PAGE_SIZE ? PAGE_SIZE : len;
        memcpy(dst_data, src, bytes);

        rem -= bytes;
        pg++;
    }
}

void mm_dupe(struct mm *old, struct mm *new)
{
    new->brk = old->brk;

    struct vm_area *curr = old->vm_areas;
    while (curr) {
        anon_mmap(new, curr->start, curr->pages);
        mm_copy_from_mm(new, old, curr->start, curr->pages);
        curr = curr->next;
    }

    mm_add_kernel_mappings(new);
}

void mm_add_kernel_mappings(struct mm *mm)
{
    mm->brk = KERNEL_BRK_START;

    /* Higher half mapping of kernel at 0xffffffff80000000 */
    vmm_map_range(mm, 0xffffffff80000000, 0, 512);

    /* Map all physical memory at 0xffff888000000000 */
    vmm_map_range(mm, 0xffff888000000000, 0, 512);

    /* Kernel heap is shared between all struct mm's */
    uint64_t brk_ndx = P4_NDX(KERNEL_BRK_START);
    p4_set_entry(mm, brk_ndx, p4_get_entry(&kernel_mm, brk_ndx));
}

void *sbrk(struct mm *mm, intptr_t inc)
{
    if (inc >= 0) {
        /* Map new pages */
        uint64_t new_map_start = ALIGNUP(mm->brk, PAGE_SIZE);
        uint64_t new_map_end = ALIGNUP(mm->brk + inc, PAGE_SIZE);

        for (uint64_t addr = new_map_start; addr < new_map_end; addr += PAGE_SIZE) {
            uint64_t frame = pmm_alloc();
            vmm_map_page(mm, addr, frame);
        }

    } else {
        /* Unmap newly unused pages */
        uintptr_t new_unmap_start = ALIGNUP(mm->brk, PAGE_SIZE);
        uintptr_t new_unmap_end = ALIGNUP(mm->brk + inc, PAGE_SIZE);

        for (int addr = new_unmap_start; addr >= new_unmap_end; addr -= PAGE_SIZE) {
            pmm_free(vmm_get_phys(mm, addr));
            vmm_unmap_page(mm, addr);
        }
    }

    mm->brk += inc;
    return (void *)(mm->brk - inc);
}
