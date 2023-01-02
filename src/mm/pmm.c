#include "pmm.h"

#include <stdbool.h>

#include "defs.h"
#include "string.h"
#include "util.h"

#define BOOT_IDENTITY_MAP_LIMIT 0x200000
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

void pmm_set_mmap(struct multiboot_tag_mmap *mmap)
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

uint64_t find_free_page(struct mmap_region *region)
{
    for (size_t i = 0; i < region->pages; i++) {
        if (region_get_bit(region, i))
            continue;

        region_set_bit(region, i);
        return (uint64_t)region->start + i * PAGE_SIZE;
    }

    return 0;
}

void pmm_init_generic(uint64_t cutoff)
{
    for (size_t i = 0; i < n_regions; i++) {
        /*
         * We may assume here that no region straddles the cutoff given the
         * region splitting logic in pmm_set_mmap.
         */
        if ((uint64_t)regions[i].start > cutoff)
            break;

        size_t len = regions[i].pages * PAGE_SIZE;

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

void pmm_init_low()
{
    pmm_init_generic(BOOT_IDENTITY_MAP_LIMIT);
}

void pmm_init_high()
{
    pmm_init_generic(0xFFFFFFFFFFFFFFFF);
}

uint64_t pmm_alloc_low()
{
    uint64_t addr = 0;
    for (size_t i = 0; i < n_regions; i++) {
        if ((uint64_t)regions[i].start > BOOT_IDENTITY_MAP_LIMIT)
            break;

        if ((addr = find_free_page(&regions[i])) != 0)
            break;
    }

    return addr;
}

void pmm_free(uint64_t phys)
{
    if (phys & 0xFFF)
        panic("attempting to free non-paged-aligned region");

    for (size_t i = 0; i < n_regions; i++) {
        struct mmap_region *region = &regions[i];
        if (!(phys > (uint64_t)region->start &&
              ((uint64_t)region->start + (region->pages * PAGE_SIZE)) < phys))
            continue;

        region_unset_bit(region, (phys - (uint64_t)region->start) / PAGE_SIZE);
    }
}
