#include "mm.h"

#include <stdbool.h>
#include <stdint.h>

#include "arena.h"
#include "defs.h"
#include "multiboot2.h"
#include "printf.h"
#include "string.h"
#include "util.h"

#define VIRT_TO_PHYS(x) (x - KERNEL_TEXT_BASE)

#define PGDIR_NDX(x) (((uintptr_t)x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t)x) >> 12) & 0xFFF)

#define INIT_ARENA_BASE ((void *)0xB0000000)

/* Bootstrap page tables */
uint32_t pgdir[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_text[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_arena[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));

struct mboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
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

static void set_cr3(void *addr)
{
    asm volatile("movl %0, %%cr3" : : "r"(VIRT_TO_PHYS(addr)));
}

static void mm_map_kernel()
{
    /* Map kernel text */
    uintptr_t curr = (uintptr_t)KERN_START;
    while (curr < (uintptr_t)KERN_END) {
        pgtbl_text[PGTBL_NDX(curr)] = curr | 0x3;
        curr += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(KERNEL_TEXT_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_text) | 0x3;
}

static void mm_map_init_arena(struct multiboot_tag_mmap *mmap, uintptr_t *size)
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
     * the zero page we can use. Check to see if it overlaps the kernel.
     */
    uintptr_t region_start = entry->addr;
    size_t region_end = (uintptr_t)entry->addr + entry->len;
    uintptr_t arena_start = 0;
    size_t arena_size = 0;
    if (KERN_START <= region_end && KERN_START >= region_start) {
        /* Kernel mapping starts somewhere in region */
        arena_start = region_start;
        arena_size = region_start - KERN_START;
    }

    if (KERN_END >= region_start && KERN_END <= region_end) {
        /* Kernel mapping ends somewhere in region */
        if (region_end - KERN_END > arena_size) {
            /* KERN_END is a better place to start (more size) */
            arena_start = KERN_END;
            arena_size = region_end - KERN_END;
        }
    }

    *size = arena_size;

    printf("using available region at %p of size %x for init arena\n", arena_start, arena_size);

    /*
     * Map ARENA_HDR_NPAGES pages at the start of the arena into kernel memory.
     * This will contain the arena header.
     */
    uintptr_t curr_phys = ALIGNUP(arena_start, PAGE_SIZE);
    uintptr_t curr_virt = (uintptr_t)INIT_ARENA_BASE;
    uintptr_t end_phys = arena_start + (PAGE_SIZE * ARENA_HDR_NPAGES);
    while (curr_phys < end_phys) {
        pgtbl_arena[PGTBL_NDX(curr_virt)] = curr_phys | 0x3;
        curr_phys += PAGE_SIZE;
        curr_virt += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(INIT_ARENA_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_arena) | 0x3;
}

void mm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);
    struct multiboot_mmap_entry *init_arena_region;
    size_t init_arena_size;

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    mm_print_mmap(mmap);

    mm_map_kernel();
    mm_map_init_arena(mmap, &init_arena_size);

    set_cr3(pgdir);

    mm_arena_setup((void *)INIT_ARENA_BASE, init_arena_size);
}
