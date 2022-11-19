#include "vmm.h"

#include "defs.h"
#include "mm.h"
#include "multiboot2.h"
#include "pmm.h"
#include "util.h"

#define KERNEL_TEXT_BASE 0xffffffff80000000
#define V_TO_P(x) (((uint64_t) (x)) - KERNEL_TEXT_BASE)

#define PHYS_MAPPING_START 0xffff888000000000

#define PAGE_PRESENT    1 << 0
#define PAGE_WRITE      1 << 1

#define __aligned4k __attribute__((aligned(0x1000)))

uint64_t kernel_p4[512] __aligned4k;

uint64_t kernel_p3_upper[512] __aligned4k;
uint64_t kernel_p3_lower[512] __aligned4k;

uint64_t kernel_p2_upper[512] __aligned4k;
uint64_t kernel_p2_lower[512] __aligned4k;

uint64_t kernel_p1[512] __aligned4k;

void vmm_setup_phys_mapping()
{
#define ENTRY(x) (((uint64_t) x) | (PAGE_PRESENT | PAGE_WRITE))

    /* Identity map first 2 MiB */
    kernel_p4[0] = ENTRY(V_TO_P(&kernel_p3_lower[0]));
    kernel_p3_lower[0] = ENTRY(V_TO_P(&kernel_p2_lower[0]));
    kernel_p2_lower[0] = ENTRY(V_TO_P(&kernel_p1[0]));

    for (uint64_t i = 0; i < 512; i ++)
        kernel_p1[i] = ENTRY(i * 0x1000);

    /*
     * Map kernel code segment at 0xffffffff80000000
     *
     * Entry 511 in P4
     * Entry 510 in P3
     */
    kernel_p4[511] = ENTRY(V_TO_P(&kernel_p3_upper[0]));
    kernel_p3_upper[510] = ENTRY(V_TO_P(&kernel_p2_upper[0]));
    kernel_p2_upper[0] = ENTRY(V_TO_P(&kernel_p1[0]));

    asm volatile("mov %0, %%cr3" : : "r" (V_TO_P(&kernel_p4)));
#undef ENTRY
}

void *sbrk(intptr_t increment)
{
    return (void *)0;
}

void vmm_init()
{
    vmm_setup_phys_mapping();
}
