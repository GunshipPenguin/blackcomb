#ifndef __DEFS_H
#define __DEFS_H

#include <stdint.h>

extern uintptr_t __kernel_start_phys;
extern uintptr_t __kernel_end_phys;

#define KERN_START ((uintptr_t)&__kernel_start_phys)
#define KERN_END ((uintptr_t)&__kernel_end_phys)

#define KERNEL_TEXT_BASE 0xC0000000

#define PAGE_MASK 0xFFF
#define PAGE_SIZE 4096

#define PGTBL_NENTRIES 1024

#endif /* __DEFS_H */
