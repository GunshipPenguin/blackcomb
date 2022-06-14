#ifndef __ARENA_H
#define __ARENA_H

#include <stdint.h>
#include <stddef.h>

/* Number of pages we map for an arena header. If we need more pages than this
 * due to the size of the arena, we panic */
#define ARENA_HDR_NPAGES 5

struct arena_hdr;

void pgframe_free(struct arena_hdr *arena, uintptr_t phys);
uintptr_t pgframe_alloc(struct arena_hdr *arena);

void pgframe_arena_init(void *arena_start, uintptr_t arena_start_phys, size_t size);

#endif /* __ARENA_H */
