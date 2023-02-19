#include "exec.h"
#include "defs.h"
#include "elf.h"
#include "ext2.h"
#include "kmalloc.h"
#include "pmm.h"
#include "string.h"
#include "syscalls.h"
#include "util.h"
#include "vmm.h"

void map_segment(struct mm *mm, void *elf, Elf64_Phdr *ph)
{
    for (int i = 0; i < DIV_CEIL(ph->p_memsz, PAGE_SIZE); i++) {
        uint64_t frame = pmm_alloc();

        uint64_t pg_start = (ph->p_vaddr + (i * PAGE_SIZE));
        vmm_map_page(mm, pg_start, frame);
        memcpy(pg_start, ((char *)elf) + ph->p_offset, ph->p_filesz);
    }
}

void map_elf(struct mm *mm, struct ext2_fs *fs, struct ext2_ino *file)
{
    void *buf = ext2_read_file(fs, file);
    Elf64_Ehdr *ehdr = buf;

    Elf64_Phdr *ph_arr = (Elf64_Phdr *)(((char *)buf) + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *ph = ph_arr + i;

        if (ph->p_type == PT_LOAD) {
            map_segment(mm, buf, ph);
        }
    }

    enter_usermode(ehdr->e_entry);

    free(buf);
}
