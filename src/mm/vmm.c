#include "vmm.h"

#include "defs.h"
#include "mm.h"
#include "multiboot2.h"
#include "pmm.h"
#include "util.h"

#define PHYS_MAPPING_START 0xffff888000000000

uint64_t kernel_p4[512];
uint64_t kernel_p3[512];
uint64_t kernel_p2[512];
uint64_t kernel_p1[512];

void vmm_setup_phys_mapping()
{
}

void *sbrk(intptr_t increment)
{
    return (void *)0;
}

void vmm_init()
{
}
