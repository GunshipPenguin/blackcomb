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

struct arena_hdr {
    size_t size;

    /* Physical start address of this page frame allocation arena */
    uintptr_t start;

    /* Virtual pointer to next struct arena_hdr */
    struct arena_hdr *next;

    uint32_t bmap[];
};

void arena_set_bit(struct arena_hdr *arena, size_t i)
{
    size_t byte_i = i / 8;
    size_t bit_i = i % 8;

    arena->bmap[byte_i] = arena->bmap[byte_i] | (1 << bit_i);
}

void pgframe_free(uintptr_t phys)
{
    if (phys & 0xFFF)
        panic("attempting to free non-page-aligned region");

    struct arena_hdr *arena = (struct arena_hdr *)ARENA_BASE;
    while (arena != NULL) {
        if (!(phys > arena->start && (arena->start + arena->size) < phys))
            continue;

        /* Physical address is in this arena */
        int framei = (phys - arena->start) / PAGE_SIZE;
        size_t index = framei / sizeof(*arena->bmap);
        size_t bit_index = framei % (sizeof(*arena->bmap) * 8);

        arena->bmap[index] = arena->bmap[index] & (~1 << bit_index);
        break;
    }
}

uintptr_t pgframe_alloc()
{
    struct arena_hdr *arena = (struct arena_hdr *)ARENA_BASE;
    while (arena != NULL) {
        for (size_t i = 0; i < arena->size; i++) {
            if (arena->bmap[i] == 0xFFFFFFFF)
                continue;

            /*
             * TODO: __builtin_ctz is much faster but might not work in this
             * freestanding setup.
             */
            for (int j = 0; j < sizeof(*arena->bmap) * 8; j++) {
                if ((1 << j) & arena->bmap[i])
                    continue;

                arena->bmap[i] |= (1 << j);
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
     * Strap in...
     *
     * This function is necessarily very complicated due to it being called
     * early in the boot process to initialize a very core piece of
     * infrastructure.
     *
     * TODO Document it here....
     */

    static uint32_t tmp_pgtbl[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
    memset(tmp_pgtbl, 0, sizeof(tmp_pgtbl));
    pgdir_set_entry(PGDIR_NDX(ARENA_BASE), ((uintptr_t)&tmp_pgtbl - KERNEL_TEXT_BASE) | 0x3);

    struct multiboot_tag_mmap *mmap_orig = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);
    uint8_t mmap_copy[mmap_orig->size];
    memcpy(mmap_copy, mmap_orig, sizeof(mmap_copy));
    struct multiboot_tag_mmap *mmap = (struct multiboot_tag_mmap *) mmap_copy;

    struct arena_hdr *curr_hdr = (struct arena_hdr *)ARENA_BASE;
    uintptr_t curr_hdr_addr = ARENA_BASE;

    struct multiboot_mmap_entry *entry = mmap->entries;

    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
#define IN_KERNEL_MAPPING(x)                                                                       \
    (x >= (uintptr_t)&__kernel_start_phys && x < (uintptr_t)&__kernel_end_phys)
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr == 0)
            goto next;

        size_t hdr_size = sizeof(struct arena_hdr) + (entry->len / PAGE_SIZE);
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
            tmp_pgtbl[PGTBL_NDX(curr_hdr_addr)] = phys | 0x3;
            pages_needed--;
            curr_hdr_addr += PAGE_SIZE;
        }

        /* curr_hdr is mapped into memory and now accessible */
        memset(curr_hdr, 0, hdr_size);
        curr_hdr->size = entry->len;
        curr_hdr->start = entry->addr;

        /* Loop again and set as reserved any kernel / arena header pages */
        for (size_t pg = 0; pg < (end - start) / PAGE_SIZE; pg++) {
            size_t phys = curr_hdr->start + (pg * PAGE_SIZE);

            /* Reserve page if it corresponds to the kernel mapping */
            if (IN_KERNEL_MAPPING(phys))
                arena_set_bit(curr_hdr, pg);

            /* Reserve page if it corresponds to any page used by the header */
            for (size_t i = 0; i < PGTBL_NENTRIES; i++) {
                if (tmp_pgtbl[i] >> PAGE_SHIFT == phys)
                    arena_set_bit(curr_hdr, pg);
            }
        }


        uint32_t print_start = curr_hdr->start;
        uint32_t print_end = curr_hdr->start + curr_hdr->size;
        printf("established a page allocation region at %p of size %d\n", print_start, print_end); 

        curr_hdr->next = (struct arena_hdr *)curr_hdr_addr;
        curr_hdr = curr_hdr->next;

    next:
        entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
    }
}
