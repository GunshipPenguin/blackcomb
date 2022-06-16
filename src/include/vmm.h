#ifndef __VMM_H
#define __VMM_H

#include <stdint.h>

#define PGDIR_NDX(x) (((uintptr_t)x) >> 22)
#define PGTBL_NDX(x) ((((uintptr_t)x) >> 12) & 0xFFF)

void pgdir_set_entry(int i, uint32_t val);
void pgtbl_set_entry(int dir_i, int tbl_i, uint32_t val);
uint32_t pgdir_get_entry(int i);
uint32_t pgtbl_get_entry(int dir_i, int tbl_i);

void vmm_map_page(uintptr_t virt, uintptr_t phys);
void vmm_unmap_page(uintptr_t virt);
uintptr_t vmm_get_phys(uintptr_t virt);

void vmm_init();

void *sbrk(intptr_t increment);

#endif /* __VMM_H */
