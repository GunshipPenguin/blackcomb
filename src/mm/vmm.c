#include "vmm.h"

#include "defs.h"
#include "kmalloc.h"
#include "multiboot2.h"
#include "pmm.h"
#include "string.h"
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

uint64_t kernel_brk = KERNEL_BRK_START;

/*
 * Standalone struct mm setup to contain kernel mappings (kernel text, kernel heap, mapping of all
 * physical memory). This is initialized before any other mm infrastructure is setup, so it, and all
 * VMA structs in it need to be statically allocated here.
 */
struct mm kernel_mm;

struct vm_area_li kernel_vma_li;
struct vm_area kernel_vma;

struct vm_area_li phys_vma_li;
struct vm_area phys_vma;

struct vm_area_li kernel_heap_vma_li;
struct vm_area kernel_heap_vma;

void switch_cr3(uint64_t addr)
{
    asm volatile("mov %0, %%cr3" : : "r"(addr));
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

void vmm_map_page(struct mm *mm, uintptr_t virt, uintptr_t phys, uint64_t flags)
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
        uint64_t entry = new_p3 | flags;
        p4_set_entry(mm, p4_i, entry);
    }

    if (p3_get_entry(mm, p4_i, p3_i) == 0) {
        /* No p2 allocated here, get a free physical page */
        uintptr_t new_p2 = pmm_alloc();
        uint64_t entry = new_p2 | flags;
        p3_set_entry(mm, p4_i, p3_i, entry);
    }

    if (p2_get_entry(mm, p4_i, p3_i, p2_i) == 0) {
        /* No p1 allocated here, get a free physical page */
        uintptr_t new_p1 = pmm_alloc();
        uint64_t entry = new_p1 | flags;
        p2_set_entry(mm, p4_i, p3_i, p2_i, entry);
    }

    p1_set_entry(mm, p4_i, p3_i, p2_i, p1_i, phys | flags);
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

uint64_t prot_to_x86_prot(uint64_t prot)
{
    uint64_t ret = 0;

    if (prot & PAGE_PROT_WRITE)
        ret |= PAGE_WRITE;

    return ret;
}

void mm_insert_vma(struct mm *mm, struct vm_area *vma)
{
    struct vm_area_li *li = kmalloc(sizeof(struct vm_area_li));

    li->vma = vma;
    vma->refcnt++;

    li->prev = NULL;
    li->next = mm->vm_areas;
    if (mm->vm_areas)
        mm->vm_areas->prev = li;

    mm->vm_areas = li;
}

void vmm_map_range(
    struct mm *mm, uint64_t virt, uint64_t phys, uint64_t npages, uint64_t prot, bool user)
{
    if ((virt & PAGE_MASK) || (phys & PAGE_MASK))
        panic("mapping is not paged aligned");

    uint64_t flags = prot_to_x86_prot(prot) | (user ? PAGE_USER : 0) | PAGE_PRESENT;
    for (int pg = 0; pg < npages; pg++) {
        uint64_t off = pg * PAGE_SIZE;
        vmm_map_page(mm, virt + off, phys + off, flags);
    }
}

void anon_mmap(struct mm *mm, uint64_t start, uint64_t pages, uint64_t prot, bool user)
{
    if (start & PAGE_MASK)
        panic("mmap is not page aligned");

    uint64_t flags = prot_to_x86_prot(prot) | (user ? PAGE_USER : 0) | PAGE_PRESENT;
    for (int i = 0; i < pages; i++) {
        uint64_t off = i * PAGE_SIZE;
        uint64_t frame = pmm_alloc();
        vmm_map_page(mm, start + off, frame, flags);
    }

    struct vm_area *vma = kcalloc(1, sizeof(struct vm_area));

    vma->prot = prot;
    vma->user = user;
    vma->type = VM_AREA_REG;

    vma->start = start;
    vma->pages = pages;

    mm_insert_vma(mm, vma);
}

void anon_mmap_user(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot)
{
    anon_mmap(mm, start, pages, prot, true);
}

void anon_mmap_kernel(struct mm *mm, uint64_t start, uint64_t pages, uint8_t prot)
{
    anon_mmap(mm, start, pages, prot, false);
}

void mm_share_vma(struct mm *dst, struct mm *src, struct vm_area *vma)
{
    uint64_t flags = prot_to_x86_prot(vma->prot) | (vma->user ? PAGE_USER : 0) | PAGE_PRESENT;
    for (int pg = 0; pg < vma->pages; pg++) {
        uint64_t off = PAGE_SIZE * pg;

        uint64_t p4 = P4_NDX(vma->start + off);
        uint64_t p3 = P3_NDX(vma->start + off);
        uint64_t p2 = P2_NDX(vma->start + off);
        uint64_t p1 = P1_NDX(vma->start + off);

        uint64_t phys = p1_get_entry(src, p4, p3, p2, p1);
        vmm_map_page(dst, vma->start + off, phys, flags);
    }

    mm_insert_vma(dst, vma);
}

void mm_copy_from_mm(struct mm *dst, struct mm *src, uint64_t start, uint64_t pages)
{
    if (start & PAGE_MASK)
        panic("copy_physmem: start is not page aligned");

    for (uint64_t pg = 0; pg < pages; pg++) {
        uint64_t off = PAGE_SIZE * pg;

        uint64_t p4 = P4_NDX(start + off);
        uint64_t p3 = P3_NDX(start + off);
        uint64_t p2 = P2_NDX(start + off);
        uint64_t p1 = P1_NDX(start + off);

        uint64_t *src_data = P_TO_V(uint64_t, p1_get_entry(src, p4, p3, p2, p1) & ~PAGE_MASK);
        uint64_t *dst_data = P_TO_V(uint64_t, p1_get_entry(dst, p4, p3, p2, p1) & ~PAGE_MASK);
        memcpy(dst_data, src_data, PAGE_SIZE);
    }
}

void mm_copy_from_buf(struct mm *dst, void *src, uint64_t start, size_t len)
{
    if (start & PAGE_MASK)
        panic("copy_physmem: start is not page aligned");

    size_t rem = len;
    uint64_t pg = 0;
    while (rem > 0) {
        uint64_t off = PAGE_SIZE * pg;

        uint64_t p4 = P4_NDX(start + off);
        uint64_t p3 = P3_NDX(start + off);
        uint64_t p2 = P2_NDX(start + off);
        uint64_t p1 = P1_NDX(start + off);

        uint64_t *dst_data = P_TO_V(uint64_t, p1_get_entry(dst, p4, p3, p2, p1) & ~PAGE_MASK);

        size_t bytes = len > PAGE_SIZE ? PAGE_SIZE : len;
        memcpy(dst_data, src, bytes);

        rem -= bytes;
        pg++;
    }
}

void kernel_mm_init(struct mm *mm)
{
    kernel_mm.vm_areas = &kernel_vma_li;

    /* Higher half mapping of kernel at 0xffffffff80000000 */
    kernel_vma.prot = PAGE_PROT_READ | PAGE_PROT_WRITE;
    kernel_vma.user = false;
    kernel_vma.type = VM_AREA_KERNELTEXT;
    kernel_vma.start = KERNEL_TEXT_BASE;
    kernel_vma.pages = 512; /* TODO: Be more intelligent than blindly mapping 512 pages */
    vmm_map_range(mm, kernel_vma.start, 0, kernel_vma.pages, PAGE_PROT_READ | PAGE_PROT_WRITE,
                  false);

    kernel_vma_li.vma = &kernel_vma;
    kernel_vma_li.next = &phys_vma_li;
    kernel_vma_li.prev = NULL;

    /* Mapping of all physical memory at 0xffff888000000000 */
    phys_vma.prot = PAGE_PROT_READ | PAGE_PROT_WRITE;
    phys_vma.user = false;
    phys_vma.type = VM_AREA_PHYSMEM;
    phys_vma.start = PHYS_MAPPING_START;
    phys_vma.pages = 512; /* TODO: Be more intelligent than blindly mapping 512 pages */
    vmm_map_range(mm, phys_vma.start, 0, phys_vma.pages, PAGE_PROT_READ | PAGE_PROT_WRITE, false);

    phys_vma_li.vma = &phys_vma;
    phys_vma_li.next = &kernel_heap_vma_li;
    phys_vma_li.prev = &kernel_vma_li;

    /* Kernel heap */
    kernel_heap_vma.prot = PAGE_PROT_READ | PAGE_PROT_WRITE;
    kernel_heap_vma.user = false;
    kernel_heap_vma.type = VM_AREA_KERNELHEAP;
    kernel_heap_vma.start = KERNEL_BRK_START;
    kernel_heap_vma.pages = 32;

    uint64_t flags = prot_to_x86_prot(kernel_heap_vma.prot) | PAGE_PRESENT;
    for (int i = 0; i < kernel_heap_vma.pages; i++) {
        uint64_t off = i * PAGE_SIZE;
        uint64_t frame = pmm_alloc();
        vmm_map_page(mm, kernel_heap_vma.start + off, frame, flags);
    }

    kernel_heap_vma_li.vma = &kernel_heap_vma;
    kernel_heap_vma_li.next = NULL;
    kernel_heap_vma_li.prev = &phys_vma_li;
}

void mm_init(struct mm *mm)
{
    struct vm_area_li *curr = kernel_mm.vm_areas;
    while (curr) {
        mm_share_vma(mm, &kernel_mm, curr->vma);
        curr = curr->next;
    }
}

struct mm *mm_new()
{
    void *mm = kcalloc(1, sizeof(struct mm));
    mm_init(mm);
    return mm;
}

struct mm *mm_dupe(struct mm *mm)
{
    struct mm *new = mm_new();

    struct vm_area_li *curr = mm->vm_areas;
    while (curr) {
        struct vm_area *vma = curr->vma;

        switch (vma->type) {
        case VM_AREA_REG:
            anon_mmap(new, vma->start, vma->pages, vma->prot, vma->user);
            mm_copy_from_mm(new, mm, vma->start, vma->pages);
            break;
        case VM_AREA_PHYSMEM:
        case VM_AREA_KERNELTEXT:
        case VM_AREA_KERNELHEAP:
            mm_share_vma(new, mm, vma);
            break;
        default:
            panic("unknown vm_area_type %d", vma->type);
        }
        curr = curr->next;
    }

    return new;
}

void mm_free(struct mm *mm)
{
    /* TODO: Implement */
    return;
}

void *sbrk(intptr_t inc)
{
    if (inc == 0)
        panic("sbrk(0), likely kernel bug");

    kernel_brk += inc;

    if (kernel_brk < KERNEL_BRK_START)
        panic("sbrk brought kernel break to below KERNEL_BRK_START");

    return (void *)(kernel_brk - inc);
}
