#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "io.h"
#include "string.h"
#include "vgaterm.h"

#define VGATERM_WIDTH 80
#define VGATERM_HEIGHT 25
#define VGATERM_BUF_START ((uint16_t *)0xffffffff800b8000)

#define VGATERM_ENTRY(c, color) (uint16_t)(c | color << 8)
#define VGATERM_COLOR(fg, bg) ((uint8_t)(fg | bg << 4))
#define VGATERM_ADDR(x, y) (((y)*VGATERM_WIDTH) + (x))

size_t vgaterm_y;
size_t vgaterm_x;
uint8_t vgaterm_color;
uint16_t *vgaterm_buf;

/* Make printf go to the vga terminal */
void _putchar(char c)
{
    vgaterm_putchar(c);
}

static void vgaterm_scroll()
{
    for (size_t row = 0; row < VGATERM_HEIGHT; row++) {
        for (size_t col = 0; col < VGATERM_WIDTH; col++) {
            vgaterm_buf[VGATERM_ADDR(col, row)] = vgaterm_buf[VGATERM_ADDR(col, row + 1)];
        }
    }

    for (size_t col = 0; col < VGATERM_WIDTH; col++) {
        vgaterm_buf[VGATERM_ADDR(col, VGATERM_HEIGHT)] = 0;
    }
}

static void vgaterm_newline()
{
    if (vgaterm_y == VGATERM_HEIGHT - 1)
        vgaterm_scroll();
    else
        vgaterm_y++;

    vgaterm_x = 0;
}

void vgaterm_set_cursor(int x, int y)
{
    uint16_t pos = vgaterm_y * VGATERM_WIDTH + vgaterm_x;

    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vgaterm_putchar(char c)
{
    if (c == '\n') {
        vgaterm_newline();
        return;
    }

    vgaterm_buf[vgaterm_y * VGATERM_WIDTH + vgaterm_x] = VGATERM_ENTRY(c, vgaterm_color);
    if (++vgaterm_x == VGATERM_WIDTH) {
        vgaterm_x = 0;
        if (++vgaterm_y == VGATERM_HEIGHT) {
            vgaterm_y = 0;
        }
    }

    vgaterm_set_cursor(vgaterm_x, vgaterm_y);
}

void vgaterm_setcolor(enum vga_color fg, enum vga_color bg)
{
    vgaterm_color = VGATERM_COLOR(fg, bg);
}

void vgaterm_print(const char *s)
{
    for (size_t i = 0; i < strlen(s); i++)
        vgaterm_putchar(s[i]);
}

void vgaterm_clear(void)
{
    vgaterm_y = 0;
    vgaterm_x = 0;
    vgaterm_color = VGATERM_COLOR(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vgaterm_buf = VGATERM_BUF_START;

    for (size_t y = 0; y < VGATERM_HEIGHT; y++) {
        for (size_t x = 0; x < VGATERM_WIDTH; x++) {
            const size_t index = y * VGATERM_WIDTH + x;
            vgaterm_buf[index] = VGATERM_ENTRY(' ', vgaterm_color);
        }
    }
}
