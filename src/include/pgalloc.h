#ifndef __ARENA_H
#define __ARENA_H

#include "mm.h"

#include <stdint.h>
#include <stddef.h>

/* Number of pages we map for an arena header. If we need more pages than this
 * due to the size of the arena, we panic */
#define ARENA_HDR_NPAGES 5

struct arena_hdr;

void pgframe_free(uintptr_t phys);
uintptr_t pgframe_alloc();

void pgalloc_init(struct multiboot_tag_mmap *mmap);

#endif /* __ARENA_H */
