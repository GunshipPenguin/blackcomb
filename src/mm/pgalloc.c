#include "pgalloc.h"

#include "defs.h"
#include "printf.h"
#include "util.h"
#include "vmm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define ARENA_BASE 0xB0000000

/*
 * Physical memory is divided into "arenas", contigous blocks of memory that
 * can be used to fulfil page allocation requests.
 */
struct arena {
    size_t size;

    /* Physical start address of this page frame allocation arena */
    uintptr_t start;

    /* Virtual pointer to next struct arena */
    struct arena *next;

    uint32_t bmap[];
};

bool arena_get_bit(struct arena *arena, size_t i)
{
    return !!(arena->bmap[i / 32] & (1 << (i % 32)));
}

void arena_set_bit(struct arena *arena, size_t i)
{
    arena->bmap[i / 32] |= 1 << (i % 32);
}

void arena_unset_bit(struct arena *arena, size_t i)
{
    arena->bmap[i / 32] &= ~1 << (i % 32);
}

void pgframe_free(uintptr_t phys)
{
    if (phys & 0xFFF)
        panic("attempting to free non-page-aligned region");

    struct arena *arena = (struct arena *)ARENA_BASE;
    while (arena != NULL) {
        if (!(phys > arena->start && (arena->start + arena->size) < phys))
            continue;

        arena_unset_bit(arena, (phys - arena->start) / PAGE_SIZE);
        break;
    }
}

uintptr_t pgframe_alloc()
{
    struct arena *arena = (struct arena *)ARENA_BASE;
    while (arena != NULL) {
        for (size_t i = 0; i < arena->size; i++) {
            if (arena->bmap[i] == 0xFFFFFFFF)
                continue;

            /*
             * TODO: __builtin_ctz is much faster but might not work in this
             * freestanding setup.
             */
            for (int j = 0; j < sizeof(*arena->bmap) * 8; j++) {
                if (arena_get_bit(arena, i * 8 + j))
                    continue;

                arena_set_bit(arena, i * 8 + j);
                return arena->start + PAGE_SIZE * ((i * sizeof(*arena->bmap)) + j);
            }
        }

        arena = arena->next;
    }

    panic("out of memory");
}

void pgalloc_init(struct mboot_info *mboot)
{
    /*
     * This function initializes the physical memory allocator. It's therefore
     * rather complicated due to having to work around the fact that absolutely
     * no other MM infrastructure is setup when it's called.
     *
     * The goal here is to go through all available regions of physical memory
     * and setup the page frame allocator to be able to allocate memory from
     * them.
     *
     * To do this, we need to setup a struct arena for each region. We map each
     * struct arena in the region of physical memory it corresponds to, using
     * the first n free pages in the region to map it where n is the size of the
     * struct arena + bitmap. We then mark all pages in the region taken up by
     * the arena metadata as reserved so they're never allocated.
     *
     * Arguably this could be done with static data in the kernel binary, but
     * would be wasteful on machines with a small amount of memory.
     */

    /*
     * This page table is used to map all the arena metadata, we need to alloc
     * it statically here due to not yet having a page frame allocator.
     */
    static uint32_t arena_meta_pgtbl[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
    memset(arena_meta_pgtbl, 0, sizeof(arena_meta_pgtbl));
    pgdir_set_entry(PGDIR_NDX(ARENA_BASE),
                    STATIC_DATA_VIRT_TO_PHYS((uintptr_t)&arena_meta_pgtbl) | 0x3);

    /*
     * The multiboot2 information is mapped somewhere in physical memory
     * (usually right after the kernel). We of course can't overwrite it with
     * arena metadata while we're bootstrapping, so copy it into a VLA on the
     * stack and use that instead. After this function returns, mboot should
     * be assumed to be garbage.
     */
    struct multiboot_tag_mmap *mmap_orig = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);
    uint8_t mmap_copy[mmap_orig->size];
    memcpy(mmap_copy, mmap_orig, sizeof(mmap_copy));
    struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *)mmap_copy;

    struct arena *curr_arena = (struct arena *)ARENA_BASE;
    uintptr_t curr_virt_page = ARENA_BASE;
    struct multiboot_mmap_entry *entry = mmap->entries;
    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
#define IN_KERNEL_MAPPING(x)                                                                       \
    (x >= (uintptr_t)&__kernel_start_phys && x < (uintptr_t)&__kernel_end_phys)
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr == 0)
            goto next;

        size_t hdr_size = sizeof(struct arena) + (entry->len / PAGE_SIZE);
        size_t pages_needed = ALIGNUP(hdr_size, PAGE_SIZE) / PAGE_SIZE;

        uintptr_t start = ALIGNUP(entry->addr, PAGE_SIZE);
        size_t end = (uintptr_t)entry->addr + entry->len;

        /* Map enough pages in this block of memory to encompass the arena header */
        for (uintptr_t phys = start; phys < end; phys += PAGE_SIZE) {
            if (pages_needed == 0)
                break; /* Mapped enough for the header */

            if (IN_KERNEL_MAPPING(phys))
                continue; /* Overlaps kernel text in physical mem, skip this page */

            /* Page is usable, map it */
            arena_meta_pgtbl[PGTBL_NDX(curr_virt_page)] = phys | 0x3;
            pages_needed--;
            curr_virt_page += PAGE_SIZE;
        }

        /* curr_arena is mapped into memory and now accessible */
        memset(curr_arena, 0, hdr_size);
        curr_arena->size = entry->len;
        curr_arena->start = entry->addr;

        /* Loop again and set as reserved any kernel / arena header pages */
        for (size_t pg = 0; pg < (end - start) / PAGE_SIZE; pg++) {
            size_t phys = curr_arena->start + (pg * PAGE_SIZE);

            /* Reserve page if it corresponds to the kernel mapping */
            if (IN_KERNEL_MAPPING(phys)) {
                arena_set_bit(curr_arena, pg);
                continue;
            }

            /* Reserve page if it corresponds to any page used by the header */
            for (size_t i = 0; i < PGTBL_NENTRIES; i++) {
                if (arena_meta_pgtbl[i] >> PAGE_SHIFT == phys) {
                    arena_set_bit(curr_arena, pg);
                    goto next_reserv;
                }
            }

        next_reserv:;
        }

        printf("established a page allocation region at %p of size %d\n", curr_arena->start,
               curr_arena->size);

        curr_arena->next = (struct arena *)curr_virt_page;
        curr_arena = curr_arena->next;

    next:
        entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
    }
}
