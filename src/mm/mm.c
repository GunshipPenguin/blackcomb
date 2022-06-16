#include "mm.h"

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "kmalloc.h"
#include "multiboot2.h"
#include "pgalloc.h"
#include "printf.h"
#include "string.h"
#include "util.h"
#include "vmm.h"

#define INIT_ARENA_BASE 0xB0000000

#define VIRT_TO_PHYS(x) ((x)-KERNEL_TEXT_BASE)

/* Used to bootstrap the initial mm arena, then discarded */
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

static void mm_map_init_arena(struct multiboot_tag_mmap *mmap, uintptr_t *phys, size_t *size)
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
        panic("Could not find memory region to use for init pgalloc arena");

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
    *phys = arena_start;

    printf("using available region at %p of size %x for init pgalloc arena\n", arena_start,
           arena_size);

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
    pgdir_set_entry(PGDIR_NDX(INIT_ARENA_BASE), VIRT_TO_PHYS((uintptr_t)pgtbl_arena) | 0x3);
}

void mm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);
    struct multiboot_mmap_entry *init_arena_region;
    uintptr_t init_arena_phys;
    size_t init_arena_size;

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    mm_print_mmap(mmap);
    mm_map_init_arena(mmap, &init_arena_phys, &init_arena_size);

    struct arena_hdr *init_arena = (struct arena_hdr *)INIT_ARENA_BASE;
    pgframe_arena_init(init_arena, init_arena_phys, init_arena_size);
    vmm_init(init_arena);
    kmalloc_init();
}
