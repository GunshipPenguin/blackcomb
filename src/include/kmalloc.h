#ifndef __KMALLOC_H
#define __KMALLOC_H

#include <stddef.h>

void *kmalloc(size_t size);
void free(void *ptr);
void kmalloc_init();

#endif
