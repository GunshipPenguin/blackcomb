#include "ata.h"
#include "ext2.h"
#include "gdt.h"
#include "int.h"
#include "kmalloc.h"
#include "mm.h"
#include "printf.h"
#include "string.h"
#include "vgaterm.h"
#include "vmm.h"

int kernel_main(uintptr_t mboot_info)
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vgaterm_print("> Blackcomb\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    printf("Initializing MM\n");
    mm_init((void *)mboot_info);

    printf("Initializing GDT\n");
    gdt_init();

    printf("Initializing IDT\n");
    idt_init();

    printf("Mounting /\n");
    struct ext2_fs *fs = ext2_mount();

    printf("Contents of /\n");
    ext2_ls(fs, "/");

    struct ext2_ino *in;
    ext2_namei(fs, &in, "/file.txt");

    char *contentbuf = kmalloc(fs->block_size);
    ext2_getblock(fs, in, contentbuf, 0);
    printf("Contents of /file.txt: %s", contentbuf);

    printf("Idling....\n");
    for (;;) {
    }
}
