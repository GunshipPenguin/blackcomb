#include "exit.h"
#include "sched.h"

int do_wait(struct task_struct *t, int *wstatus)
{
    struct task_struct *child;

retry:
    child = t->children;
    while (child) {
        if (child->state == TASK_ZOMBIE)
            break;

        child = child->sibling;
    }

    if (!child) {
        /* No zombie children, sleep until a child exits */
        current->state = TASK_WAITING;
        schedule_sleep();
        goto retry;
    }

    *wstatus = t->exit_status;
    int ret = child->pid;
    task_free(child);
    return ret;
}

int do_exit(struct task_struct *t, int status)
{
    t->state = TASK_ZOMBIE;
    t->exit_status = status;

    if (t->parent->state == TASK_WAITING) {
        sched_rr_insert_proc(t->parent);
        t->parent->state = TASK_RUNNING;
    }

    schedule_sleep();
    return 0; /* Never hit */
}
