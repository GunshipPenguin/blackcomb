#include "tty.h"
#include "kmalloc.h"
#include "sched.h"
#include "util.h"

struct tty *global_tty;

void tty_init()
{
    global_tty = kcalloc(1, sizeof(*global_tty));
}

void tty_send_char(char c)
{
    if (global_tty->bufi > TTY_BUF_SIZE)
        panic("tty buf exceeded");

    if (c == '\n') {
        if (global_tty->waiting_task) {
            sched_rr_insert_proc(global_tty->waiting_task);
            global_tty->waiting_task = NULL;
        }

        global_tty->bufi = 0;
        goto print;
    }

    global_tty->buf[global_tty->bufi++] = c;

print:
    printf("%c", c);
}

void wait_tty(char *buf, size_t n) {
    if (global_tty->waiting_task)
        panic("tty cannot manage >1 waiting task");

    global_tty->waiting_task = current;
    schedule_sleep();
}
