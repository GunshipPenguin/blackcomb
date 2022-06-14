#include "vmm.h"
#include "defs.h"
#include "mm.h"
#include "util.h"

#include "pgalloc.h"

#include <stdint.h>

#define BRK_START 0x90000000
#define PGDIR_RECURSIVE_BASE 0xFFC00000

/* Page directory is mapped to the very last page in memory */
#define PGDIR_ENTRY_ADDR(i) (((uint32_t *)(PGDIR_RECURSIVE_BASE + (PAGE_SIZE * 1023))) + i)

/* Each directory entry corresponds to 1024 pages */
#define PGTBL_ENTRY_ADDR(dir_i, tbl_i)                                                             \
    ((uint32_t *)((PGDIR_RECURSIVE_BASE + (PAGE_SIZE * dir_i))) + tbl_i)

uintptr_t curr_brk;
struct arena_hdr *arena;

static void pgdir_set_entry(int i, uint32_t val)
{
    uint32_t *addr = PGDIR_ENTRY_ADDR(i);
    *addr = val;
}

static void pgtbl_set_entry(int dir_i, int tbl_i, uint32_t val)
{
    uint32_t *addr = PGTBL_ENTRY_ADDR(dir_i, tbl_i);
    *addr = val;
}

static uint32_t pgdir_get_entry(int i)
{
    uint32_t *addr = PGDIR_ENTRY_ADDR(i);
    return *addr;
}

static uint32_t pgtbl_get_entry(int dir_i, int tbl_i)
{
    uint32_t *addr = PGTBL_ENTRY_ADDR(dir_i, tbl_i);
    return *addr;
}

uintptr_t vmm_get_phys(uintptr_t virt)
{
    size_t dir_i = PGDIR_NDX(virt);
    size_t tbl_i = PGTBL_NDX(virt);

    return pgtbl_get_entry(dir_i, tbl_i) & ~0xFFF;
}

void vmm_map_page(uintptr_t virt, uintptr_t phys)
{
    size_t dir_i = PGDIR_NDX(virt);
    size_t tbl_i = PGTBL_NDX(virt);

    if (pgdir_get_entry(dir_i) == 0) {
        /* No page table allocated here, get a free physical page */
        uintptr_t new_frame = pgframe_alloc(arena);
        uint32_t entry = new_frame | 0x3;
        pgdir_set_entry(dir_i, entry);
    }

    pgtbl_set_entry(dir_i, tbl_i, phys | 0x3);
}

void vmm_unmap_page(uintptr_t virt)
{
    size_t dir_i = PGDIR_NDX(virt);
    size_t tbl_i = PGTBL_NDX(virt);

    pgtbl_set_entry(dir_i, tbl_i, 0);

    for (int i = 0; i < 1023; i++) {
        if (pgtbl_get_entry(dir_i, i) != 0)
            return;
    }

    /* Entire page table is 0, we can free the frame */
    pgframe_free(arena, pgdir_get_entry(dir_i) & ~PAGE_MASK);
    pgdir_set_entry(dir_i, 0);
}

void vmm_init(struct arena_hdr *a)
{
    curr_brk = BRK_START;
    arena = a;
}

void *sbrk(intptr_t inc)
{
    if (inc >= 0) {
        /* Map new pages */
        uintptr_t new_map_start = ALIGNUP(curr_brk, PAGE_SIZE);
        uintptr_t new_map_end = ALIGNUP(curr_brk + inc, PAGE_SIZE);

        for (int addr = new_map_start; addr < new_map_end; addr += PAGE_SIZE) {
            uintptr_t frame = pgframe_alloc(arena);
            vmm_map_page(addr, frame);
        }

    } else {
        /* Unmap newly unused pages */
        uintptr_t new_unmap_start = ALIGNUP(curr_brk, PAGE_SIZE);
        uintptr_t new_unmap_end = ALIGNUP(curr_brk + inc, PAGE_SIZE);

        for (int addr = new_unmap_start; addr >= new_unmap_end; addr -= PAGE_SIZE) {
            pgframe_free(arena, vmm_get_phys(addr));
            vmm_unmap_page(addr);
        }
    }

    curr_brk += inc;
    return (void *)(curr_brk - inc);
}
