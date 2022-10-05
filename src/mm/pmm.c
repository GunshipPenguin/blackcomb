#include "pmm.h"

#include <stdbool.h>

#include "defs.h"
#include "string.h"
#include "util.h"

#define BOOT_IDENTITY_MAP_LIMIT ((void *)0x200000)
#define MAX_REGIONS 64

struct mmap_region {
    char *start;
    size_t pages;
};

size_t n_regions = 0;
struct mmap_region regions[MAX_REGIONS];

bool region_get_bit(struct mmap_region *region, size_t i)
{
    return !!(region->start[i / sizeof(region->start)] & (1 << (i % sizeof(region->start))));
}

void region_set_bit(struct mmap_region *region, size_t i)
{
    region->start[i / sizeof(region->start)] |= 1 << (i % sizeof(region->start));
}

void region_unset_bit(struct mmap_region *region, size_t i)
{
    region->start[i / sizeof(region->start)] &= ~1 << (i % sizeof(region->start));
}

static void copy_regions(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry *entry = mmap->entries;
    size_t i = 0;

    memset(regions, 0, sizeof(regions));

    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            regions[i].start = (char *)entry->addr;
            regions[i].pages = entry->len / PAGE_SIZE;
            i++;
        }

        entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
    }

    n_regions = i;
}

void *find_free_page(struct mmap_region *region)
{
    for (size_t i = 0; i < region->pages; i++) {
        if (region_get_bit(region, i))
            continue;

        region_set_bit(region, i);
        return region->start + i * PAGE_SIZE;
    }

    return NULL;
}

void init_arenas_low()
{
    for (size_t i = 0; i < n_regions; i++) {
        if (regions[i].start > BOOT_IDENTITY_MAP_LIMIT)
            break;

        size_t len = regions[i].pages * PAGE_SIZE;

        if (len > BOOT_IDENTITY_MAP_LIMIT)
            len = BOOT_IDENTITY_MAP_LIMIT;

        size_t bmap_size = ((len / PAGE_SIZE) / 8);
        memset(regions[i].start, 0, bmap_size);

        /*
         * Set bits corresponding to the bitmap itself so pages containing the bitmap are
         * never returned to fulfill an allocation request.
         */
        for (int j = 0; j < ALIGNUP(bmap_size, PAGE_SIZE); j++)
            region_set_bit(&regions[i], j);
    }
}

void *pmm_alloc_low()
{
    void *addr = NULL;
    for (size_t i = 0; i < n_regions; i++) {
        if (regions[i].start > BOOT_IDENTITY_MAP_LIMIT)
            break;

        if ((addr = find_free_page(&regions[i])) != 0)
            break;
    }

    return addr;
}

void pmm_init_low(struct multiboot_tag_mmap *mmap)
{
    copy_regions(mmap);
    init_arenas_low();
}
