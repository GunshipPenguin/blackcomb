#include "ata.h"
#include "tty.h"
#include "exec.h"
#include "ps2.h"
#include "ext2.h"
#include "gdt.h"
#include "int.h"
#include "kmalloc.h"
#include "pmm.h"
#include "printf.h"
#include "sched.h"
#include "string.h"
#include "syscalls.h"
#include "vgaterm.h"
#include "vmm.h"
#include <stdarg.h>

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf("[ ");
    vgaterm_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    printf("INFO");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf(" ] ");

    vprintf(fmt, args);
}

void log_ok(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    printf("[ ");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    printf(" OK ");
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

void kernel_main(uintptr_t mboot_info)
{
    banner();

    pmm_init((void *)mboot_info);
    log_ok("Page frame allocator initialized\n");

    kernel_mm_init(&kernel_mm);
    log_ok("Kernel page tables initialized\n");

    switch_cr3(kernel_mm.p4);
    log_ok("Kernel page tables switched\n");

    kmalloc_init();
    log_ok("Kmalloc initialized\n");

    gdt_init();
    log_ok("GDT initialized\n");

    idt_init();
    log_ok("IDT initialized\n");

    ps2_init();
    log_ok("keyboard initialized\n");

    tty_init();
    log_ok("tty initialized\n");

    syscall_enable();
    log_ok("Syscalls enabled\n");

    rootfs = ext2_mount_rootfs();
    log_ok("Mounted /\n");

    printf("Contents of /\n");
    ext2_ls(rootfs, "/");

    struct ext2_ino *in;
    ext2_namei(rootfs, &in, "/file.txt");
    char *contentbuf = ext2_read_file(rootfs, in);
    printf("Contents of /file.txt: %s", contentbuf);
    free(contentbuf);

    log_info("attempting to exec /init\n");
    start_init();
}
