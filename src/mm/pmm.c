#include "pmm.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "multiboot2.h"
#include "string.h"
#include "util.h"
#include "vmm.h"

#define BOOT_IDENTITY_MAP_LIMIT 0x200000
#define MAX_REGIONS 64
#define BITS(x) ((x)*8)

struct mboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};

struct mmap_region {
    char *start;
    size_t pages;
};

size_t n_regions = 0;
struct mmap_region regions[MAX_REGIONS];

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

bool region_get_bit(struct mmap_region *region, size_t i)
{
    char *start = P_TO_V(char, region->start);
    return !!(start[i / BITS(sizeof(*start))] & (1 << (i % BITS(sizeof(*start)))));
}

void region_set_bit(struct mmap_region *region, size_t i)
{
    char *start = P_TO_V(char, region->start);
    start[i / BITS(sizeof(*start))] |= 1 << (i % BITS(sizeof(*start)));
}

void region_unset_bit(struct mmap_region *region, size_t i)
{
    char *start = P_TO_V(char, region->start);
    start[i / BITS(sizeof(*start))] &= ~1 << (i % BITS(sizeof(*start)));
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

void pmm_init_regions()
{
    for (size_t i = 0; i < n_regions; i++) {
        size_t bmap_size = (regions[i].pages + 8 - 1) / 8;
        char *start = P_TO_V(char, regions[i].start);
        memset(start, 0, bmap_size);

        /*
         * Set bits corresponding to the bitmap itself so pages containing the bitmap are
         * never returned to fulfill an allocation request.
         */
        for (int j = 0; j < (bmap_size + PAGE_SIZE - 1) / PAGE_SIZE; j++)
            region_set_bit(&regions[i], j);
    }
}

void pmm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    pmm_set_mmap(mmap);
    pmm_init_regions();
}

uint64_t pmm_alloc()
{
    uint64_t addr = 0;
    for (size_t i = 0; i < n_regions; i++) {
        if ((addr = find_free_page(&regions[i])) != 0)
            break;
    }

    void *virt = P_TO_V(void, addr);
    memset(virt, 0, PAGE_SIZE);

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
