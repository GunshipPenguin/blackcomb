#include "kmalloc.h"
#include "defs.h"
#include "string.h"
#include "util.h"
#include "vmm.h"

void *heap_base = NULL;

#define INIT_HEAP_SIZE (PAGE_SIZE * 4)

struct block {
    size_t size;
    int in_use;
    struct block *next;
    struct block *prev;
};

extern struct mm kernel_mm;

static struct block *split_block(size_t size, struct block *victim)
{
    if (victim->size - size <= sizeof(struct block)) {
        /*
         * Not enough space to fit another block in, just return the victim
         * without splitting.
         */
        return victim;
    }

    struct block *new_block = (struct block *)(((char *)victim) + sizeof(struct block) + size);
    new_block->size = victim->size - size - sizeof(struct block);
    new_block->in_use = 0;
    new_block->next = victim->next;
    new_block->prev = victim;

    victim->size = size;

    if (victim->next)
        victim->next->prev = new_block;

    victim->next = new_block;

    return victim;
}

void *kmalloc(size_t size)
{
    if (size == 0)
        panic("malloc of size 0, likely kernel bug");

    struct block *prev = NULL;
    struct block *curr = heap_base;
    struct block *target = NULL;
    while (curr != NULL) {
        if (curr->in_use)
            goto next;

        if (curr->size == size) {
            target = curr;
            break;
        }

        if (curr->size >= size + sizeof(struct block)) {
            target = split_block(size, curr);
            break;
        }

    next:
        prev = curr;
        curr = curr->next;
    }

    if (!target) {
        /* No free block found, use sbrk to get more memory */
        target = sbrk(sizeof(struct block) + size);
        target->in_use = 1;

        target->next = NULL;
        target->prev = prev;
        prev->next = target;
    }

    target->in_use = 1;
    return target + 1;
}

void *kcalloc(size_t nmemb, size_t size)
{
    size_t sz = nmemb * size;

    void *mem = kmalloc(sz);
    memset(mem, 0, sz);

    return mem;
}

void free(void *ptr)
{
    struct block *block = ((struct block *)ptr) - 1;
    block->in_use = 0;

    /* Coalesce back */
    if (block->prev && !block->prev->in_use) {
        block->prev->size += block->size + sizeof(struct block);
        block->prev->next = block->next;

        if (block->next)
            block->next->prev = block->prev;

        block = block->prev;
    }

    /* Coalesce forward */
    if (block->next && !block->next->in_use) {
        block->size += block->next->size + sizeof(struct block);
        block->next = block->next->next;

        if (block->next)
            block->next->prev = block;
    }
}

void kmalloc_init()
{
    heap_base = sbrk(INIT_HEAP_SIZE);

    struct block *first = heap_base;
    first->size = INIT_HEAP_SIZE - sizeof(struct block);
    first->in_use = 0;
    first->next = NULL;
    first->prev = NULL;
}
