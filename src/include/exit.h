#ifndef __EXIT_H
#define __EXIT_H

#include "sched.h"

int do_wait(struct task_struct *t, int *wstatus);
int do_exit(struct task_struct *t, int status);

#endif /* __EXIT_H */
