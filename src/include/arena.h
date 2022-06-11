#ifndef __ARENA_H
#define __ARENA_H

#include <stddef.h>

/* Number of pages we map for an arena header. If we need more pages than this
 * due to the size of the arena, we panic */
#define ARENA_HDR_NPAGES 5

void mm_arena_setup(void *start, size_t size);

#endif /* __ARENA_H */
