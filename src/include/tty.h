#ifndef __TTY_H
#define __TTY_H

#define TTY_BUF_SIZE 1024

struct tty {
    int bufi;
    char buf[TTY_BUF_SIZE];

    struct task_struct *waiting_task;
};

extern struct tty *global_tty;

void tty_init();
void tty_send_char(char c);

#endif /* __TTY_H */
