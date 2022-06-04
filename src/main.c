#include "vgaterm.h"

int kernel_main(uint32_t multiboot_info)
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    vgaterm_print("=========\n");
    vgaterm_print("Evergreen\n");
    vgaterm_print("=========\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    vgaterm_print("Hello paging world");
    for (;;) {
    }
}

void _putchar(char c)
{
}
