#include "vmm.h"

#include "defs.h"
#include "mm.h"
#include "multiboot2.h"
#include "pmm.h"
#include "util.h"

#define P4_NDX(x >> 39) & 0x1FFF
#define P3_NDX(x >> 30) & 0x1FFF
#define P2_NDX(x >> 21) & 0x1FFF
#define P1_NDX(x >> 12) & 0x1FFF

#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P_STATIC(x) (((uint64_t) (x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000
#define P_TO_V(type, x) ((type *) (PHYS_MAPPING_START + (uint64_t) x));

#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITE      (1 << 1)
#define PAGE_SIZE_FLAG  (1 << 7)

#define BRK_START 0x90000000

#define __aligned4k __attribute__((aligned(0x1000)))

uintptr_t curr_brk;

uint64_t kernel_p4[512] __aligned4k;

uint64_t kernel_p3_upper[512] __aligned4k;
uint64_t kernel_p3_lower[512] __aligned4k;

uint64_t kernel_p2_upper[512] __aligned4k;
uint64_t kernel_p2_lower[512] __aligned4k;

uint64_t kernel_p1[512] __aligned4k;

uint64_t kernel_p3_physmem[512] __aligned4k;

void vmm_setup_phys_mapping(uint64_t physmem_size)
{
#define ENTRY(x) (((uint64_t) x) | (PAGE_PRESENT | PAGE_WRITE))

    /* Identity map first 2 MiB */
    kernel_p4[0] = ENTRY(V_TO_P_STATIC(&kernel_p3_lower[0]));
    kernel_p3_lower[0] = ENTRY(V_TO_P_STATIC(&kernel_p2_lower[0]));
    kernel_p2_lower[0] = ENTRY(V_TO_P_STATIC(&kernel_p1[0]));

    for (uint64_t i = 0; i < 512; i ++)
        kernel_p1[i] = ENTRY(i * 0x1000);

    /*
     * Map kernel code segment at 0xffffffff80000000
     *
     * Entry 511 in P4
     * Entry 510 in P3
     */
    kernel_p4[511] = ENTRY(V_TO_P_STATIC(&kernel_p3_upper[0]));
    kernel_p3_upper[510] = ENTRY(V_TO_P_STATIC(&kernel_p2_upper[0]));
    kernel_p2_upper[0] = ENTRY(V_TO_P_STATIC(&kernel_p1[0]));

    /*
     * Map all physical memory at ffff888000000000
     *
     * Entry 273 in P4
     *
     * We use 1GB pages (using P3 to map) to map physical
     * mem.
     */
    kernel_p4[273] = ENTRY(V_TO_P_STATIC(&kernel_p3_physmem[0]));
    
    uint64_t mapped = 0;
    for (uint64_t i = 0; i < 512; i++) {
        /* 
         * 1<<30 == 1 GiB
         *
         * PAGE_SIZE_FLAG tells the CPU this references a page
         * itself and not a P2 entry 
         */
        kernel_p3_physmem[i] = ENTRY(i * (1<<30)) | PAGE_SIZE_FLAG;
        mapped += 1<<30;

        if (mapped >= physmem_size)
            break;
    }

    if (mapped < physmem_size)
        panic("Could not map all physical memory");

    asm volatile("mov %0, %%cr3" : : "r" (V_TO_P_STATIC(&kernel_p4)));
#undef ENTRY
}

void vmm_init()
{
    vmm_setup_phys_mapping(1<<20);
    curr_brk = BRK_START;
}

void pgdir_set_entry(int i, uint32_t val)
{
    uint32_t *addr = PGDIR_ENTRY_ADDR(i);
    *addr = val;
}

void pgtbl_set_entry(int dir_i, int tbl_i, uint32_t val)
{
    uint32_t *addr = PGTBL_ENTRY_ADDR(dir_i, tbl_i);
    *addr = val;
}

uint64_t p4_get_entry(int i)
{
    return kernel_p4[i];
}

uint64_t p3_get_entry(int p4_i, int i)
{
    uint64_t *p3_base_addr = P_TO_V(p4_get_entry(p4_i) & ~PAGE_MASK);
    return p3_base_addr[i]; 
}

uint64_t p2_get_entry(int p4_i, int p3_i, int i)
{
    uint64_t *p2_base_addr = P_TO_V(p3_get_entry(p4_i, p3_i) & ~PAGE_MASK);
    return p3_base_addr[i]; 
}

uint64_t p1_get_entry(int p4_i, int p3_i, int p2_i, int i)
{
    uint64_t *p1_base_addr = P_TO_V(p2_get_entry(p4_i, p3_i, p2_i) & ~PAGE_MASK);
    return p1_base_addr[i]; 
}

void p4_set_entry(int i, uint64_t val)
{
    kernel_p4[i] = val;
}

void p3_set_entry(int p4_i, int i, uint64_t val)
{
    uint64_t *p3_base_addr = P_TO_V(p4_get_entry(p4_i) & ~PAGE_MASK);
    p3_base_addr[i] = val;
}

void p2_set_entry(int p4_i, int p3_i, int i, uint64_t val)
{
    uint64_t *p2_base_addr = P_TO_V(p3_get_entry(p4_i, p3_i) & ~PAGE_MASK);
    p2_base_addr[i] = val;
}

void p1_set_entry(int p4_i, int p3_i, int p2_i, int i, uint64_t val)
{
    uint64_t *p1_base_addr = P_TO_V(p2_get_entry(p4_i, p3_i, p2_i) & ~PAGE_MASK);
    p2_base_addr[i] = val;
}

uintptr_t vmm_get_phys(uintptr_t virt)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    return p1_get_entry(p4_i, p3_i, p2_i, p1_i) & ~PAGE_MASK;
}

void vmm_map_page(uintptr_t virt, uintptr_t phys)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    if (p4_get_entry(p4_i) == 0) {
        /* No p3 allocated here, get a free physical page */
        uintptr_t new_p3 = pgframe_alloc();
        uint32_t entry = new_frame | PAGE_PRESENT | PAGE_WRITE;
        p4_set_entry(p4_i, new_p3);
    }

    if (p3_get_entry(p4_i, p3_i) == 0) {
        /* No p2 allocated here, get a free physical page */
        uintptr_t new_p2 = pgframe_alloc();
        uint32_t entry = new_frame | PAGE_PRESENT | PAGE_WRITE;
        p3_set_entry(p4_i, p3_i, new_p2);
    }

    if (p2_get_entry(p4_i, p3_i, p2_i) == 0) {
        /* No p1 allocated here, get a free physical page */
        uintptr_t new_p1 = pgframe_alloc();
        uint32_t entry = new_frame | PAGE_PRESENT | PAGE_WRITE;
        p2_set_entry(p4_i, p3_i, p2_i, new_p1);
    }

    p1_set_entry(p4_i, p3_i, p2_i, p1_i, phys | PAGE_PRESENT | PAGE_WRITE);
}

void vmm_unmap_page(uintptr_t virt)
{
    uint64_t p4_i = P4_NDX(virt);
    uint64_t p3_i = P3_NDX(virt);
    uint64_t p2_i = P2_NDX(virt);
    uint64_t p1_i = P1_NDX(virt);

    p1_set_entry(p4_i, p3_i, p2_i, p1_i, 0);

    /* TODO: This works but is slow as hell, do something less naive */

    for (int i = 0; i < 1023; i++) {
        if (p1_get_entry(p4_i, p3_i, p2_i, i) != 0)
            goto maybe_free_p2;
    }
    
    /* Entire P1 is 0, we can free the frame */
    pgframe_free(p2_get_entry(p4_i, p3_i, p2_i) & ~PAGE_MASK);
    p2_set_entry(p4_i, p3_i, p2_i, 0);

maybe_free_p2:
    for (int i = 0; i < 1023; i++) {
        if (p2_get_entry(p4_i, p3_i, i) != 0)
            goto maybe_free_p3;
    }
    
    /* Entire P2 is 0, we can free the frame */
    pgframe_free(p3_get_entry(p4_i, p3_i) & ~PAGE_MASK);
    p3_set_entry(p4_i, p3_i, 0);

maybe_free_p3:
    for (int i = 0; i < 1023; i++) {
        if (p3_get_entry(p4_i, i) != 0)
            goto out;
    }
    
    /* Entire P3 is 0, we can free the frame */
    pgframe_free(p4_get_entry(p4_i) & ~PAGE_MASK);
    p4_set_entry(p4_i, 0);

out:
    return;
}

void *sbrk(intptr_t inc)
{
    if (inc >= 0) {
        /* Map new pages */
        uintptr_t new_map_start = ALIGNUP(curr_brk, PAGE_SIZE);
        uintptr_t new_map_end = ALIGNUP(curr_brk + inc, PAGE_SIZE);

        for (int addr = new_map_start; addr < new_map_end; addr += PAGE_SIZE) {
            uintptr_t frame = pgframe_alloc();
            vmm_map_page(addr, frame);
        }

    } else {
        /* Unmap newly unused pages */
        uintptr_t new_unmap_start = ALIGNUP(curr_brk, PAGE_SIZE);
        uintptr_t new_unmap_end = ALIGNUP(curr_brk + inc, PAGE_SIZE);

        for (int addr = new_unmap_start; addr >= new_unmap_end; addr -= PAGE_SIZE) {
            pgframe_free(vmm_get_phys(addr));
            vmm_unmap_page(addr);
        }
    }

    curr_brk += inc;
    return (void *)(curr_brk - inc);
}
