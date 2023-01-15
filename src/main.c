#include "gdt.h"
#include "int.h"
#include "kmalloc.h"
#include "mm.h"
#include "printf.h"
#include "string.h"
#include "vgaterm.h"
#include "vmm.h"

void syscall_enable();
void do_sysret();

int kernel_main(uintptr_t mboot_info)
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vgaterm_print("> Blackcomb\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    mm_init((void *)mboot_info);
    gdt_init();

    idt_init();

    char *buf = kmalloc(1024);
    printf("kmalloc'd a region of size 1024 at %p\n", buf);
    strcpy(buf, "Hello World!");
    printf("This string is in a kmalloc'd region -> \"%s\"\n", buf);

    syscall_enable();
    do_sysret();

    for (;;) {
    }
}
