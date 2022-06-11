#include "arena.h"
#include "defs.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define NORDERS 5

struct arena_hdr {
    size_t size;
    uint32_t *bmap_ptrs[NORDERS];
    uint32_t bmap[];
};

static void fill_arena_bmap_ptrs(struct arena_hdr *hdr)
{
    uint32_t *curr_bmap = hdr->bmap;
    size_t block_sz = PAGE_SIZE;
    for (int i = 0; i < NORDERS; i++) {
        hdr->bmap_ptrs[i] = curr_bmap;

        /* number of bytes = arena_sz / block_size / 8 */
        curr_bmap += (hdr->size / block_sz) >> 3;
        block_sz <<= 1;
    }
}

void mm_arena_setup(void *start, size_t size)
{
    struct arena_hdr *hdr = start;
    memset(start, 0, ARENA_HDR_NPAGES * PAGE_SIZE);

    hdr->size = size;
    fill_arena_bmap_ptrs(hdr);
}
