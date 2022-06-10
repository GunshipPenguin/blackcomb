#include "mm.h"

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "multiboot2.h"
#include "printf.h"
#include "util.h"

#define VIRT_TO_PHYS(x) (x - KERNEL_TEXT_BASE)

#define PGDIR_NDX(x) (((uintptr_t)x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t)x) >> 12) & 0xFFF)

#define INIT_ARENA_BASE 0xB0000000

/* Bootstrap page tables */
uint32_t pgdir[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_text[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_arena[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));

struct mboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};

struct arena_hdr {
    /* TODO: Fill this in */
    int nonce;
};

static void *mboot_find_tag(struct mboot_info *mboot, uint32_t tag)
{
    struct multiboot_tag *curr = mboot->tags;

    while (1) {
        if (curr->type == tag)
            break;
        else if (curr->type == MULTIBOOT_TAG_TYPE_END)
            break;

        uintptr_t next = ALIGNUP((uintptr_t)curr + curr->size, 8);
        curr = (struct multiboot_tag *)next;
    }

    if (curr->type == MULTIBOOT_TAG_TYPE_END)
        return NULL;

    return (struct multiboot_tag_mmap *)curr;
}
static void mm_print_mmap(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry *entry = mmap->entries;

    printf("Multiboot2 memory map:\n");
    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        char *type;
        switch (entry->type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
            type = "AVAIL";
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            type = "RESV";
            break;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            type = "ACPI";
            break;
        case MULTIBOOT_MEMORY_NVS:
            type = "NVS";
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            type = "BAD";
            break;
        default:
            panic("bad memory type");
        }

        uintptr_t start = entry->addr;
        uintptr_t end = entry->addr + entry->len;
        printf("entry %p-%p %s\n", start, end, type);
        entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
    }
}

static void set_cr3(uintptr_t addr)
{
    asm volatile("movl %0, %%cr3" : : "r"(VIRT_TO_PHYS(addr)));
}

static void mm_map_kernel()
{
    /* Map kernel text */
    uintptr_t curr = (uintptr_t)&__kernel_start_phys;
    while (curr < (uintptr_t)&__kernel_end_phys) {
        pgtbl_text[PGTBL_NDX(curr)] = curr | 0x3;
        curr += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(KERNEL_TEXT_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_text) | 0x3;
}

static void mm_setup_init_arena(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry *entry = mmap->entries;
    bool found = false;

    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr == 0) {
            entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
            continue;
        }

        found = true;
        break;
    }

    if (!found)
        panic("Could not find memory region to use for init arena");

    /*
     * Ok, we've found a region of available physical memory not starting at
     * the zero page we can use. Map the first sizeof(struct arena_hdr) bytes
     * into virtual memory and use it as our initial arena.
     */
    uintptr_t curr = ALIGNUP((uintptr_t)entry->addr, PAGE_SIZE);
    uintptr_t end = (uintptr_t)entry->addr + sizeof(struct arena_hdr);
    while (curr < end) {
        pgtbl_arena[PGTBL_NDX(curr)] = curr | 0x3;
        curr += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(INIT_ARENA_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_arena) | 0x3;
}

void mm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    mm_print_mmap(mmap);

    mm_map_kernel();
    mm_setup_init_arena(mmap);

    set_cr3((uintptr_t)pgdir);
}
