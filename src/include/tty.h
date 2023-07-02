#ifndef __TTY_H
#define __TTY_H

#include "ps2.h"

#include <stdint.h>
#include <stddef.h>

#define TTY_BUF_SIZE 1024

struct tty {
    int bufi;
    char buf[TTY_BUF_SIZE];

    struct task_struct *waiting_task;

    struct file *stdin;
    struct file *stdout;
};

extern struct tty *global_tty;

void tty_init();
void tty_send_key(enum ps2_key c);
int64_t tty_read(char *buf, size_t count);
void tty_install_stdin_stdout(struct task_struct *t);

#endif /* __TTY_H */
