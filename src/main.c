#include "printf.h"
#include "vgaterm.h"
#include "mm.h"

int kernel_main(uint32_t mboot_info)
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vgaterm_print("> Blackcomb\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    mboot_info = 0xA0000000 + (mboot_info & 0xfff);
    mm_init((void *) mboot_info);

    for (;;) {
    }
}

void _putchar(char c)
{
    vgaterm_putchar(c);
}
