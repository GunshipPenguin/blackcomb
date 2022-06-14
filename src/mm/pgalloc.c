#include "pgalloc.h"
#include "defs.h"
#include "util.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct arena_hdr {
    size_t size;
    uintptr_t start;
    uint32_t bmap[];
};

void pgframe_free(struct arena_hdr *arena, uintptr_t phys)
{
    if (phys & 0xFFF)
        panic("attempting to free non-page-aligned region");

    int framei = (phys - arena->start) / PAGE_SIZE;
    size_t index = framei / sizeof(*arena->bmap);
    size_t bit_index = framei % (sizeof(*arena->bmap) * 8);

    uint32_t entry = arena->bmap[index];
    arena->bmap[index] = arena->bmap[index] & (~1 << bit_index);
}

uintptr_t pgframe_alloc(struct arena_hdr *arena)
{
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

    panic("out of memory");
}

void pgframe_arena_init(void *arena_start, uintptr_t arena_start_phys, size_t size)
{
    struct arena_hdr *hdr = arena_start;

    memset(arena_start, 0, ARENA_HDR_NPAGES * PAGE_SIZE);

    hdr->size = size;
    hdr->start = arena_start_phys + (ARENA_HDR_NPAGES * PAGE_SIZE);
}
