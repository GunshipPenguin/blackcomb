#include "mm.h"

#include <stdint.h>

#include "defs.h"
#include "multiboot2.h"
#include "printf.h"
#include "util.h"

#define VIRT_TO_PHYS(x) (x - KERNEL_TEXT_BASE)

#define PGDIR_NDX(x) (((uintptr_t) x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t) x) >> 12) & 0xFFF)

/* Bootstrap page tables */
uint32_t pgdir[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_text[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));
uint32_t pgtbl_arena[PGTBL_NENTRIES] __attribute__((aligned(PAGE_SIZE)));

struct multiboot_info {
    int32_t total_size;
    int32_t reserved;
    struct multiboot_tag tags[];
};

static struct multiboot_tag_mmap *mboot_find_mmap(void *mboot_info_start)
{
    struct multiboot_info *mboot = mboot_info_start;
    struct multiboot_tag *curr = mboot->tags;

    while (1) {
        if (curr->type == MULTIBOOT_TAG_TYPE_MMAP)
            break;
        else if (curr->type == MULTIBOOT_TAG_TYPE_END)
            break;
        uintptr_t next = ALIGNUP((uintptr_t)curr + curr->size, 8);
        curr = (struct multiboot_tag *)next;
    }

    if (curr->type == MULTIBOOT_TAG_TYPE_END)
        panic("No MMAP multiboot tag found");

    return (struct multiboot_tag_mmap *)curr;
}
static void mm_print_mmap(void *mboot_info_start)
{
    struct multiboot_tag_mmap *mmap = mboot_find_mmap(mboot_info_start);
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

static void mm_setup_kernel_text_map()
{
    uintptr_t curr = (uintptr_t)&__kernel_start_phys;
    while (curr < (uintptr_t) &__kernel_end_phys) {
        pgtbl_text[PGTBL_NDX(curr)] = curr | 0x3;
        curr += PAGE_SIZE;
    }

    pgdir[PGDIR_NDX(KERNEL_TEXT_BASE)] = VIRT_TO_PHYS((uintptr_t)pgtbl_text) | 0x3;
}

static void set_cr3(uintptr_t addr)
{
    asm volatile("movl %0, %%cr3" : : "r"(VIRT_TO_PHYS(addr)));
}

static void mm_setup_arenas(void *mboot_info_start)
{
    struct multiboot_tag_mmap *mmap = mboot_find_mmap(mboot_info_start);
    struct multiboot_mmap_entry *entry = mmap->entries;

    while ((uintptr_t)entry < (uintptr_t)mmap + mmap->size) {
        if (entry->type != MULTIBOOT_MEMORY_AVAILABLE || entry->addr == 0) {
            entry = (struct multiboot_mmap_entry *)((uintptr_t)entry + mmap->entry_size);
            continue;
        }

        /*
         * Ok, we've found some free memory not starting at the zero-page, use
         * it for our init arena.
         */
        
    }
}

void mm_init(void *mboot_info_start)
{
    mm_print_mmap(mboot_info_start);

    mm_setup_kernel_text_map();
    set_cr3((uintptr_t)pgdir);
}
