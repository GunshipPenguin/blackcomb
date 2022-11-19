#include "mm.h"

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "kmalloc.h"
#include "multiboot2.h"
#include "pmm.h"
#include "printf.h"
#include "string.h"
#include "util.h"
#include "vmm.h"

void *mboot_find_tag(struct mboot_info *mboot, uint32_t tag)
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

void mm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    mm_print_mmap(mmap);

    pmm_init_low(mmap);

    vmm_init(mmap);

    printf("%p\n", pmm_alloc_low());
    printf("%p\n", pmm_alloc_low());
    printf("%p\n", pmm_alloc_low());
    printf("%p\n", pmm_alloc_low());
    printf("%p\n", pmm_alloc_low());
}
