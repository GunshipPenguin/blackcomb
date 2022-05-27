#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "string.h"
#include "vgaterm.h"

#define VGATERM_WIDTH 80
#define VGATERM_HEIGHT 25
#define VGATERM_BUF_START ((uint16_t *)0xB8000)

size_t vgaterm_y;
size_t vgaterm_x;
uint8_t vgaterm_color;
uint16_t *vgaterm_buf;

static inline uint16_t vgaterm_entry(unsigned char uc, uint8_t color)
{
    return (uint16_t)uc | (uint16_t)color << 8;
}

static inline uint16_t vgaterm_address(size_t row, size_t col)
{
    return row * VGATERM_WIDTH + col;
}

static void vgaterm_putentryat(char c, uint8_t color, size_t x, size_t y)
{
    const size_t index = y * VGATERM_WIDTH + x;
    vgaterm_buf[index] = vgaterm_entry(c, color);
}

static void vgaterm_scroll()
{
    for (size_t row = 0; row < VGATERM_HEIGHT; row++) {
        for (size_t col = 0; col < VGATERM_WIDTH; col++) {
            vgaterm_buf[vgaterm_address(row, col)] = vgaterm_buf[vgaterm_address(row + 1, col)];
        }
    }
}

static void vgaterm_newline()
{
    if (vgaterm_y == VGATERM_HEIGHT) {
        vgaterm_scroll();
    } else {
        vgaterm_y++;
        vgaterm_x = 0;
    }
}

static void vgaterm_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
        vgaterm_putchar(data[i]);
}

uint8_t vgaterm_entry_color(enum vga_color fg, enum vga_color bg)
{
    return fg | bg << 4;
}

void vgaterm_putchar(char c)
{
    if (c == '\n') {
        vgaterm_newline();
        return;
    }

    vgaterm_putentryat(c, vgaterm_color, vgaterm_x, vgaterm_y);
    if (++vgaterm_x == VGATERM_WIDTH) {
        vgaterm_x = 0;
        if (++vgaterm_y == VGATERM_HEIGHT) {
            vgaterm_y = 0;
        }
    }
}

void vgaterm_setcolor(uint8_t color)
{
    vgaterm_color = color;
}

void vgaterm_writestring(const char *data)
{
    vgaterm_write(data, strlen(data));
}

void vgaterm_initialize(void)
{
    vgaterm_y = 0;
    vgaterm_x = 0;
    vgaterm_color = vgaterm_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vgaterm_buf = VGATERM_BUF_START;

    for (size_t y = 0; y < VGATERM_HEIGHT; y++) {
        for (size_t x = 0; x < VGATERM_WIDTH; x++) {
            const size_t index = y * VGATERM_WIDTH + x;
            vgaterm_buf[index] = vgaterm_entry(' ', vgaterm_color);
        }
    }
}
