#include <stddef.h>

#include "kmalloc.h"
#include "ps2.h"
#include "sched.h"
#include "string.h"
#include "tty.h"
#include "util.h"

unsigned char key_map[256] = {
    [KEY_A] = 'a',     [KEY_B] = 'b',         [KEY_C] = 'c',      [KEY_D] = 'd',     [KEY_E] = 'e',
    [KEY_F] = 'f',     [KEY_G] = 'g',         [KEY_H] = 'h',      [KEY_I] = 'i',     [KEY_J] = 'j',
    [KEY_K] = 'k',     [KEY_L] = 'l',         [KEY_M] = 'm',      [KEY_N] = 'n',     [KEY_O] = 'o',
    [KEY_P] = 'p',     [KEY_Q] = 'q',         [KEY_R] = 'r',      [KEY_S] = 's',     [KEY_T] = 't',
    [KEY_U] = 'u',     [KEY_V] = 'v',         [KEY_W] = 'w',      [KEY_X] = 'x',     [KEY_Y] = 'y',
    [KEY_Z] = 'z',     [KEY_1] = '1',         [KEY_2] = '2',      [KEY_3] = '3',     [KEY_4] = '4',
    [KEY_5] = '5',     [KEY_6] = '6',         [KEY_7] = '7',      [KEY_8] = '8',     [KEY_9] = '9',
    [KEY_SPACE] = ' ', [KEY_SEMICOLON] = ';', [KEY_ENTER] = '\n', [KEY_SLASH] = '/',
};

struct tty *global_tty;

void tty_init()
{
    global_tty = kcalloc(1, sizeof(*global_tty));
}

void tty_send_key(enum ps2_key key)
{
    if (global_tty->bufi > TTY_BUF_SIZE)
        panic("tty buf exceeded");

    char c = key_map[key];
    global_tty->buf[global_tty->bufi++] = c;

    if (key == KEY_ENTER) {
        if (global_tty->waiting_task) {
            sched_rr_insert_proc(global_tty->waiting_task);
            global_tty->waiting_task = NULL;
        }

        goto print;
    }

print:
    printf("%c", c);
}

int64_t tty_read(char *buf, size_t n)
{
    if (global_tty->waiting_task)
        panic("tty cannot manage >1 waiting task");

    global_tty->waiting_task = current;

    schedule_sleep();

    int64_t nbytes = global_tty->bufi > n ? n : global_tty->bufi;

    memcpy(buf, global_tty->buf, nbytes);
    global_tty->bufi = 0;

    return nbytes;
}
