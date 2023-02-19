#include "ata.h"
#include "exec.h"
#include "ext2.h"
#include "gdt.h"
#include "int.h"
#include "pmm.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"
#include "syscalls.h"
#include "vgaterm.h"
#include "vmm.h"
#include <stdarg.h>

void log_ok(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf("[ ");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf("OK");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf(" ] ");

    vprintf(fmt, args);
}

void banner()
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    printf("> Blackcomb\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
}

struct mm kernel_mm;

int kernel_main(uintptr_t mboot_info)
{
    banner();

    pmm_init((void *)mboot_info);
    log_ok("Page frame allocator initialized\n");

    init_kernel_mm(&kernel_mm);
    log_ok("Kernel page tables initialized\n");

    switch_cr3(kernel_mm.p4);
    log_ok("Kernel page tables switched\n");

    kmalloc_init();
    log_ok("Kmalloc initialized\n");

    gdt_init();
    log_ok("GDT initialized\n");

    idt_init();
    log_ok("IDT initialized\n");

    syscall_enable();
    log_ok("Syscalls enabled\n");

    struct ext2_fs *fs = ext2_mount();
    log_ok("Mounted /\n");

    printf("Contents of /\n");
    ext2_ls(fs, "/");

    struct ext2_ino *in;
    ext2_namei(fs, &in, "/file.txt");

    char *contentbuf = ext2_read_file(fs, in);
    printf("Contents of /file.txt: %s", contentbuf);
    free(contentbuf);

    struct ext2_ino *init_ino;
    ext2_namei(fs, &init_ino, "/init");
    map_elf(&kernel_mm, fs, init_ino);

    printf("Idling....\n");
    for (;;) {
    }
}
