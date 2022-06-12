#include "arena.h"
#include "defs.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define NORDER 5

struct arena_hdr {
    size_t size;
    void *start;

    /*
     * Bit == 0 -> This block (and all subblocks) are free.
     * Bit == 1 -> _At least one page_ of this region is allocated
     */
    uint32_t *bmaps[NORDER];
    size_t bmap_lens[NORDER];
};

static bool bmap_entry(uint32_t *bmap, int i)
{
    size_t byte = i / sizeof(*bmap);
    size_t bit = i % sizeof(*bmap);
    return bmap[byte] >> (1 << bit);
}

static void set_bmap_entry(uint32_t *bmap, int i)
{
    size_t byte = i / sizeof(*bmap);
    size_t bit = i % sizeof(*bmap);
    bmap[byte] = bmap[byte] | (1 << bit);
}

static void unset_bmap_entry(uint32_t *bmap, int i)
{
    size_t byte = i / sizeof(*bmap);
    size_t bit = i % sizeof(*bmap);
    bmap[byte] = bmap[byte] & (~(1 << bit));
}

static int find_free_block(struct arena_hdr *hdr, int order)
{
    for (int i = 0; i < hdr->bmap_lens[order]; i++) {
        if (hdr->bmaps[order][i] == 0xFFFFFFFF)
            continue;

        uint32_t val = hdr->bmaps[order][i];
        for (int j = 0; j < 8; j++) {
            if ((bmap >> j) & 1)
                return (i * 32) + j;
        }
    }

    return -1;
}

static void *pgframe_address(struct arena_hdr *hdr, int order, int i)
{
    return ((char *) hdr->start) + ((1 << order) * i);
}

static void *pgframe_alloc(struct arena_hdr *hdr, int frame_order)
{
    int blocki = find_free_block(hdr, frame_order);
    if (blocki == -1)
        panic("out of memory");

    /* Set every higher order block that overlaps to allocated */
    int ord = frame_order;
    int i = blocki;
    for (; ord < NORDER; ord++) {
        set_bmap_entry(hdr->bmaps[ord], i);
        i >>= 1;
    }

    return pgframe_address(hdr, frame_order, i);
}

static void pgframe_free(struct arena_hdr *hdr, int frame_order, int framei)
{
    unset_bmap_entry(hdr->bmaps[frame_order], i);

    /* Walk up the orders and coalesce buddies if possible */
    int i = framei;
    for (int ord = frame_order + 1; i < NORDER; i++) {
        int buddyi = i & 1 ? i - 1 : i + 1;

        if (bmap_entry(hdr->bmaps[ord - 1], i)) {
            unset_bmap_entry(hdr->bmaps[ord], buddyi)
        } else {
            break;
        }

        i >>= 1;
    }
}

void mm_arena_setup(void *start, size_t size)
{
    struct arena_hdr *hdr = start;
    memset(start, 0, ARENA_HDR_NPAGES * PAGE_SIZE);

    hdr->size = size;

    uint32_t *curr_bmap = hdr->bmap;
    size_t block_sz = PAGE_SIZE;
    size_t bmap_size = 0;
    for (int i = 0; i < NORDER; i++) {
        /* number of bytes = arena_sz / block_size / 8 */
        size_t curr_bmap_size = (hdr->size / block_sz) >> 3;
        bmap_size += curr_bmap_size;

        hdr->bmaps[i] = curr_bmap;
        hdr->bmap_lens[i] = bmap_size;

        curr_bmap = ((uintptr_t)curr_bmap) + bmap_size;
        block_sz <<= 1;
    }

    hdr->start = ((char *) start) + sizeof(struct arena_hdr) + bmap_size;
}
