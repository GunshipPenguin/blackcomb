#include "mm.h"
#include "printf.h"
#include "vgaterm.h"

int kernel_main(uintptr_t mboot_info, uint32_t mboot_size)
{
    vgaterm_clear();
    vgaterm_setcolor(VGA_COLOR_LIGHT_BLUE, VGA_COLOR_BLACK);
    vgaterm_print("> Blackcomb\n");
    vgaterm_setcolor(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    mm_init((void *)mboot_info);

    for (;;) {
    }
}
