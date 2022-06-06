#include "mm.h"

#include <stdint.h>

#include "multiboot2.h"
#include "printf.h"

#define ALIGNUP(val, align) ({ \
	const typeof((val)) _val = (val); \
	const typeof((align)) _align = (align); \
	(_val + (_align - 1)) & -_align; })

struct multiboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};



void panic(char *s)
{
    printf("%s\n", s);
    for (;;) {}
}

static struct multiboot_tag_mmap *mboot_find_mmap(void *mboot_info_start)
{
    struct multiboot_info *mboot = mboot_info_start;
    struct multiboot_tag *curr = mboot->tags;

    while (1) {
        if (curr->type == MULTIBOOT_TAG_TYPE_MMAP)
            break;
        else if (curr->type == MULTIBOOT_TAG_TYPE_END)
            break;
        uintptr_t next = ALIGNUP((uintptr_t) curr + curr->size, 8);
        curr = (struct multiboot_tag *) next;
    }

    if (curr->type == MULTIBOOT_TAG_TYPE_END)
        panic("No MMAP multiboot tag found");

    return (struct multiboot_tag_mmap *) curr;
}

void mm_init(void *mboot_info_start)
{
    struct multiboot_tag_mmap *mmap = mboot_find_mmap(mboot_info_start);
    struct multiboot_mmap_entry *entry = mmap->entries;

    printf("Multiboot2 memory map:\n");
    while ((uintptr_t) entry < (uintptr_t) mmap + mmap->size) {
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
        entry = (struct multiboot_mmap_entry *) ((uintptr_t) entry + mmap->entry_size);
    }
}
