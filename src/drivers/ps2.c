#include <stdbool.h>

#include "io.h"
#include "printf.h"
#include "tty.h"
#include "util.h"

enum key {
    KEY_RESERVED,
    KEY_ESC,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    KEY_BACKSPACE,
    KEY_TAB,
    KEY_Q,
    KEY_W,
    KEY_E,
    KEY_R,
    KEY_T,
    KEY_Y,
    KEY_U,
    KEY_I,
    KEY_O,
    KEY_P,
    KEY_LEFTBRACE,
    KEY_RIGHTBRACE,
    KEY_ENTER,
    KEY_LEFTCTRL,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_SEMICOLON,
    KEY_APOSTROPHE,
    KEY_GRAVE,
    KEY_LEFTSHIFT,
    KEY_BACKSLASH,
    KEY_Z,
    KEY_X,
    KEY_C,
    KEY_V,
    KEY_B,
    KEY_N,
    KEY_M,
    KEY_COMMA,
    KEY_DOT,
    KEY_SLASH,
    KEY_RIGHTSHIFT,
    KEY_KPASTERISK,
    KEY_LEFTALT,
    KEY_SPACE,
    KEY_CAPSLOCK,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_NUMLOCK,
    KEY_SCROLLLOCK,
    KEY_KP7,
    KEY_KP8,
    KEY_KP9,
    KEY_KPMINUS,
    KEY_KP4,
    KEY_KP5,
    KEY_KP6,
    KEY_KPPLUS,
    KEY_KP1,
    KEY_KP2,
    KEY_KP3,
    KEY_KP0,
    KEY_KPDOT,
    KEY_102ND,
    KEY_F11,
    KEY_F12,
    KEY_RO,
    KEY_KPENTER,
    KEY_RIGHTCTRL,
    KEY_KPSLASH,
    KEY_SYSRQ,
    KEY_RIGHTALT,
    KEY_HOME,
    KEY_UP,
    KEY_PAGEUP,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_END,
    KEY_DOWN,
    KEY_PAGEDOWN,
    KEY_INSERT,
    KEY_DELETE,
    KEY_PAUSE,
    KEY_LEFTMETA,
    KEY_RIGHTMETA,
    KEY_COMPOSE,
};

// clang-format off
unsigned char sc_map[] = {
    /* 00 */  KEY_RESERVED, KEY_F9,        KEY_RESERVED,  KEY_F5,        KEY_F3,        KEY_F1,       KEY_F2,        KEY_F12,
    /* 08 */  KEY_ESC,      KEY_F10,       KEY_F8,        KEY_F6,        KEY_F4,        KEY_TAB,      KEY_GRAVE,     KEY_F2,
    /* 10 */  KEY_RESERVED, KEY_LEFTALT,   KEY_LEFTSHIFT, KEY_RESERVED,  KEY_LEFTCTRL,  KEY_Q,        KEY_1,         KEY_F3,
    /* 18 */  KEY_RESERVED, KEY_LEFTALT,   KEY_Z,         KEY_S,         KEY_A,         KEY_W,        KEY_2,         KEY_F4,
    /* 20 */  KEY_RESERVED, KEY_C,         KEY_X,         KEY_D,         KEY_E,         KEY_4,        KEY_3,         KEY_F5,
    /* 28 */  KEY_RESERVED, KEY_SPACE,     KEY_V,         KEY_F,         KEY_T,         KEY_R,        KEY_5,         KEY_F6,
    /* 30 */  KEY_RESERVED, KEY_N,         KEY_B,         KEY_H,         KEY_G,         KEY_Y,        KEY_6,         KEY_F7,
    /* 38 */  KEY_RESERVED, KEY_RIGHTALT,  KEY_M,         KEY_J,         KEY_U,         KEY_7,        KEY_8,         KEY_F8,
    /* 40 */  KEY_RESERVED, KEY_COMMA,     KEY_K,         KEY_I,         KEY_O,         KEY_0,        KEY_9,         KEY_F9,
    /* 48 */  KEY_RESERVED, KEY_DOT,       KEY_SLASH,     KEY_L,         KEY_SEMICOLON, KEY_P,        KEY_MINUS,     KEY_F10,
    /* 50 */  KEY_RESERVED, KEY_RESERVED,  KEY_APOSTROPHE,KEY_RESERVED,  KEY_LEFTBRACE, KEY_EQUAL,    KEY_F11,       KEY_SYSRQ,
    /* 58 */  KEY_CAPSLOCK, KEY_RIGHTSHIFT,KEY_ENTER,     KEY_RIGHTBRACE,KEY_BACKSLASH, KEY_BACKSLASH,KEY_F12,       KEY_SCROLLLOCK,
    /* 60 */  KEY_DOWN,     KEY_102ND,     KEY_PAUSE,     KEY_UP,        KEY_DELETE,    KEY_END,      KEY_BACKSPACE, KEY_INSERT,
    /* 68 */  KEY_RESERVED, KEY_KP1,       KEY_RIGHT,     KEY_KP4,       KEY_KP7,       KEY_PAGEDOWN, KEY_HOME,      KEY_PAGEUP,
    /* 70 */  KEY_KP0,      KEY_KPDOT,     KEY_KP2,       KEY_KP5,       KEY_KP6,       KEY_KP8,      KEY_ESC,       KEY_NUMLOCK,
    /* 78 */  KEY_F11,      KEY_KPPLUS,    KEY_KP3,       KEY_KPMINUS,   KEY_KPASTERISK,KEY_KP9,      KEY_SCROLLLOCK,KEY_102ND,
    /* 80 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* 88 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* 90 */  KEY_RESERVED, KEY_RIGHTALT,  255,           KEY_RESERVED,  KEY_RIGHTCTRL, KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* 98 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_CAPSLOCK, KEY_RESERVED,  KEY_LEFTMETA,
    /* a0 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RIGHTMETA,
    /* a8 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_COMPOSE,
    /* b0 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* b8 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* c0 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* c8 */  KEY_RESERVED, KEY_RESERVED,  KEY_KPSLASH,   KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* d0 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* d8 */  KEY_RESERVED, KEY_RESERVED,  KEY_KPENTER,   KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* e0 */  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED,  KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* e8 */  KEY_RESERVED, KEY_END,       KEY_RESERVED,  KEY_LEFT,      KEY_HOME,      KEY_RESERVED, KEY_RESERVED,  KEY_RESERVED,
    /* f0 */  KEY_INSERT,   KEY_DELETE,    KEY_DOWN,      KEY_RESERVED,  KEY_RIGHT,     KEY_UP,       KEY_RESERVED,  KEY_PAUSE,
    /* f8 */  KEY_RESERVED, KEY_RESERVED,  KEY_PAGEDOWN,  KEY_RESERVED,  KEY_SYSRQ,     KEY_PAGEUP,   KEY_RESERVED,  KEY_RESERVED,
};
// clang-format on

unsigned char sc_char[256] = {
    [KEY_A] = 'a',     [KEY_B] = 'b',         [KEY_C] = 'c',      [KEY_D] = 'd', [KEY_E] = 'e',
    [KEY_F] = 'f',     [KEY_G] = 'g',         [KEY_H] = 'h',      [KEY_I] = 'i', [KEY_J] = 'j',
    [KEY_K] = 'k',     [KEY_L] = 'l',         [KEY_M] = 'm',      [KEY_N] = 'n', [KEY_O] = 'o',
    [KEY_P] = 'p',     [KEY_Q] = 'q',         [KEY_R] = 'r',      [KEY_S] = 's', [KEY_T] = 't',
    [KEY_U] = 'u',     [KEY_V] = 'v',         [KEY_W] = 'w',      [KEY_X] = 'x', [KEY_Y] = 'y',
    [KEY_Z] = 'z',     [KEY_1] = '1',         [KEY_2] = '2',      [KEY_3] = '3', [KEY_4] = '4',
    [KEY_5] = '5',     [KEY_6] = '6',         [KEY_7] = '7',      [KEY_8] = '8', [KEY_9] = '9',
    [KEY_SPACE] = ' ', [KEY_SEMICOLON] = ';', [KEY_ENTER] = '\n',
};

#define PS2_DATA 0x60
#define PS2_COMMAND 0x64
#define PS2_STATUS 0x64

void wait_ready()
{
    while (!(inb(PS2_STATUS) & 1)) {
    }
}

struct tty *tty;

bool release = false;
void ps2_key_in()
{
    uint8_t data = inb(PS2_DATA);

    if (release) {
        release = false;
        return;
    }

    if (data == 0xf0)
        release = true;
    else
        tty_send_char(sc_char[sc_map[data]]);
}

void ps2_set_tty(struct tty *tty)
{
    tty = tty;
}

void ps2_init()
{
    /* Disable first and second PS2 ports */
    outb(PS2_COMMAND, 0xAD);
    outb(PS2_COMMAND, 0xA7);

    /* Clear input buffer */
    inb(PS2_DATA);

    /* Perform status test */
    outb(PS2_COMMAND, 0xAA);
    wait_ready();
    if (inb(PS2_DATA) != 0x55)
        panic("PS2 selftest failed");

    /* Enable first PS2 port */
    outb(PS2_COMMAND, 0xAE);

    /* Enable interrupts on first PS2 port */
    uint8_t config = inb(0x20);
    config |= 1;
    outb(PS2_COMMAND, 0x60);
    outb(PS2_DATA, config);
}
