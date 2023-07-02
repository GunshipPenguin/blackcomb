#ifndef __FORK_H
#define __FORK_H

struct task_struct *do_fork(struct task_struct *t);
struct task_struct *create_init();

#endif /* __FORK_H */
