#include <stddef.h>

#include "defs.h"
#include "elf.h"
#include "exec.h"
#include "ext2.h"
#include "kmalloc.h"
#include "pmm.h"
#include "sched.h"
#include "string.h"
#include "syscalls.h"
#include "util.h"
#include "vmm.h"

void map_segment(struct mm *mm, void *elf, Elf64_Phdr *ph)
{
    uint64_t map_addr = ph->p_vaddr & ~PAGE_MASK;
    uint64_t map_offset = ph->p_offset & ~PAGE_MASK;
    uint64_t extraspace = ph->p_vaddr & PAGE_MASK;

    anon_mmap_user(mm, map_addr, DIV_CEIL(ph->p_memsz + extraspace, PAGE_SIZE), PAGE_PROT_READ);
    mm_copy_from_buf(mm, ((char *)elf) + map_offset, map_addr, ph->p_filesz + extraspace);
}

void exec_elf(struct task_struct *task, struct ext2_fs *fs, struct ext2_ino *file)
{
    void *buf = ext2_read_file(fs, file);
    Elf64_Ehdr *ehdr = buf;
    task->mm = mm_new();

    Elf64_Phdr *ph_arr = (Elf64_Phdr *)(((char *)buf) + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *ph = ph_arr + i;

        if (ph->p_type == PT_LOAD)
            map_segment(task->mm, buf, ph);
    }
    task->regs->rip = ehdr->e_entry;

    /* Done mapping ELF sections */
    free(buf);

    anon_mmap_user(task->mm, USER_STACK_BASE, USER_STACK_PAGES, PAGE_PROT_READ | PAGE_PROT_WRITE);
    task->regs->rsp = USER_STACK_BASE + (PAGE_SIZE * USER_STACK_PAGES);
}
