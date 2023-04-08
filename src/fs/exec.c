#include "exec.h"
#include "defs.h"
#include "elf.h"
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
    for (int i = 0; i < DIV_CEIL(ph->p_memsz, PAGE_SIZE); i++) {
        uint64_t frame = pmm_alloc();

        uint64_t pg_start = (ph->p_vaddr + (i * PAGE_SIZE));
        vmm_map_page(mm, pg_start, frame);
        memcpy((void *)pg_start, ((char *)elf) + ph->p_offset, ph->p_filesz);
    }
}

struct task_struct *task_from_elf(struct ext2_fs *fs, struct ext2_ino *file)
{
    void *buf = ext2_read_file(fs, file);
    Elf64_Ehdr *ehdr = buf;

    struct task_struct *task = kmalloc(sizeof(struct task_struct));
    memset(task, 0, sizeof(struct task_struct));

    mm_add_kernel_mappings(&task->mm);

    /* Kernel mappings added, can switch to the userspace page tables now */
    uint64_t old_cr3;
    asm volatile("mov %%cr3, %0" : "=rm"(old_cr3) :);

    switch_cr3(task->mm.p4);

    Elf64_Phdr *ph_arr = (Elf64_Phdr *)(((char *)buf) + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++) {
        Elf64_Phdr *ph = ph_arr + i;

        if (ph->p_type == PT_LOAD)
            map_segment(&task->mm, buf, ph);
    }
    task->regs.rip = ehdr->e_entry;

    /* Done mapping ELF sections */
    free(buf);

    anon_mmap(&task->mm, USER_STACK_BASE, USER_STACK_PAGES);
    task->regs.rsp = USER_STACK_BASE + (PAGE_SIZE * USER_STACK_PAGES);

    anon_mmap(&task->mm, KERNEL_STACK_BASE, KERNEL_STACK_PAGES);

    switch_cr3(old_cr3);

    return task;
}
