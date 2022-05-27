#include "vgaterm.h"

int kernel_main()
{
    vgaterm_initialize();
    vgaterm_setcolor(vgaterm_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK));
    vgaterm_writestring("=========\n");
    vgaterm_writestring("Evergreen\n");
    vgaterm_writestring("=========\n");
    vgaterm_setcolor(vgaterm_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));

    vgaterm_writestring("ABCDEFGHIJKLMNOPQRSTUVWXYZ\n123456789\n");
    for (;;) {
    }
}
