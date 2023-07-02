#include "ps2.h"

#include "io.h"
#include "printf.h"
#include "tty.h"
#include "util.h"

#include <stdbool.h>

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
        tty_send_key(sc_map[data]);
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
