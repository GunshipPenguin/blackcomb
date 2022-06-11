#include "mm.h"

#include <stdbool.h>
#include <stdint.h>

#include "defs.h"
#include "multiboot2.h"
#include "printf.h"
#include "string.h"
#include "util.h"

#define VIRT_TO_PHYS(x) (x - KERNEL_TEXT_BASE)

#define PGDIR_NDX(x) (((uintptr_t)x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t)x) >> 12) & 0xFFF)

#define INIT_ARENA_BASE ((void *)0xB0000000)

#define NORDERS 5
#define ARENA_HDR_NPAGES 5

/* Bootstrap page tables */
uint32_t pgdir[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_text[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_arena[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));

struct mboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};

struct arena_hdr {
    size_t sz;
    size_t bmap_sz;
    uint32_t *bmap_ptrs[NORDERS];
    uint32_t bmap[];
};

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
static void mm_print_mmap(struct multiboot_tag_mmap *mmap)
{
    struct multiboot_mmap_entry *entry = mmap->entries;

    printf("Multiboot2 memory map:\n");
    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        char *type;
        switch (entry->type) {
        case MULTIBOOT_MEMORY_AVAILABLE:
            type = "AVAIL";
            break;
        case MULTIBOOT_MEMORY_RESERVED:
            type = "RESV";
            break;
        case MULTIBOOT_MEMORY_ACPI_RECLAIMABLE:
            type = "ACPI";
            break;
        case MULTIBOOT_MEMORY_NVS:
            type = "NVS";
            break;
        case MULTIBOOT_MEMORY_BADRAM:
            type = "BAD";
            break;
        default:
            panic("bad memory type");
        }

        uintptr_t start = entry->addr;
        uintptr_t end = entry->addr + entry->len;
        printf("entry %p-%p %s\n", start, end, type);
        entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
    }
}

static void set_cr3(void *addr)
{
    asm volatile("movl %0, %%cr3" : : "r"(VIRT_TO_PHYS(addr)));
}

static void mm_map_kernel()
{
    /* Map kernel text */
    uintptr_t curr = (uintptr_t)KERN_START;
    while (curr < (uintptr_t)KERN_END) {
        pgtbl_text[PGTBL_NDX(curr)] = curr | 0x3;
        curr += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(KERNEL_TEXT_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_text) | 0x3;
}

static size_t mm_arena_bmap_size(size_t arena_sz)
{
    size_t bmap_sz = 0;
    size_t block_sz = PAGE_SIZE;

    for (int i = 0; i < NORDERS; i++) {
        /* number of bytes = ceil(arena_sz / block_size) / 8 */
        bmap_sz += ((arena_sz / block_sz) + 7) >> 3;
        block_sz <<= 1;
    }

    return bmap_sz;
}

static void fill_arena_bmap_ptrs(struct arena_hdr *hdr)
{
    uint32_t *curr_bmap = hdr->bmap;
    size_t block_sz = PAGE_SIZE;
    for (int i = 0; i < NORDERS; i++) {
        hdr->bmap_ptrs[i] = curr_bmap;

        /* number of bytes = arena_sz / block_size / 8 */
        curr_bmap += (hdr->sz / block_sz) >> 3;
        block_sz <<= 1;
    }
}

static void mm_map_init_arena(struct multiboot_tag_mmap *mmap, struct arena_hdr *hdr)
{
    struct multiboot_mmap_entry *entry = mmap->entries;
    bool found = false;

    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr == 0) {
            entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
            continue;
        }

        found = true;
        break;
    }

    if (!found)
        panic("Could not find memory region to use for init arena");

    /*
     * Ok, we've found a region of available physical memory not starting at
     * the zero page we can use. Check to see if it overlaps the kernel.
     */
    uintptr_t region_start = entry->addr;
    uintptr_t region_end = (uintptr_t)entry->addr + entry->len;
    uintptr_t arena_start = 0;
    uintptr_t arena_size = 0;
    if (KERN_START <= region_end && KERN_START >= region_start) {
        /* Kernel mapping starts somewhere in region */
        arena_start = region_start;
        arena_size = region_start - KERN_START;
    }

    if (KERN_END >= region_start && KERN_END <= region_end) {
        /* Kernel mapping ends somewhere in region */
        if (region_end - KERN_END > arena_size) {
            /* KERN_END is a better place to start (more size) */
            arena_start = KERN_END;
            arena_size = region_end - KERN_END;
        }
    }

    printf("using available region at %p of size %x for init arena\n", arena_start, arena_size);

    /*
     * Map the first sizeof(struct arena_hdr) bytes
     * into virtual memory and use it as our initial arena.
     */
    size_t arena_bmap_sz = mm_arena_bmap_size(arena_size);
    uintptr_t curr_phys = ALIGNUP(arena_start, PAGE_SIZE);
    uintptr_t curr_virt = (uintptr_t)INIT_ARENA_BASE;
    uintptr_t end_phys = arena_start + (PAGE_SIZE * ARENA_HDR_NPAGES);
    while (curr_phys < end_phys) {
        pgtbl_arena[PGTBL_NDX(curr_virt)] = curr_phys | 0x3;
        curr_phys += PAGE_SIZE;
        curr_virt += PAGE_SIZE;
    }
    pgdir[PGDIR_NDX(INIT_ARENA_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_arena) | 0x3;

    hdr->sz = entry->len - (PAGE_SIZE * ARENA_HDR_NPAGES);
    hdr->bmap_sz = mm_arena_bmap_size(hdr->sz);
    fill_arena_bmap_ptrs(hdr);

    if (sizeof(struct arena_hdr) + hdr->bmap_sz > PAGE_SIZE * ARENA_HDR_NPAGES)
        panic("arena hdr too big");

    printf("init arena has a size of %d (bmap size %d)\n", hdr->sz, hdr->bmap_sz);
}

void mm_init(void *mboot_info_start)
{
    struct mboot_info *mboot = mboot_info_start;
    struct multiboot_tag_mmap *mmap = mboot_find_tag(mboot, MULTIBOOT_TAG_TYPE_MMAP);
    struct multiboot_mmap_entry *init_arena_region;
    struct arena_hdr init_arena;

    printf("kernel start is %p\n", KERN_START);
    printf("kernel end is %p\n", KERN_END);

    if (mmap == NULL)
        panic("Could not find multiboot2 mmap tag");

    mm_print_mmap(mmap);

    mm_map_kernel();
    mm_map_init_arena(mmap, &init_arena);

    set_cr3(pgdir);

    memset(INIT_ARENA_BASE, 0, ARENA_HDR_NPAGES * PAGE_SIZE);
    memcpy(INIT_ARENA_BASE, &init_arena, sizeof(init_arena));
    fill_arena_bmap_ptrs(&init_arena);
}
