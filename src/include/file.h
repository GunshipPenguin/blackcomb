#ifndef __FILE_H
#define __FILE_H

#include <stdint.h>
#include <stddef.h>

struct file {
    struct file_ops *ops;
    int ino;
    int refcnt;
};

struct file_ops {
    size_t (*read) (struct file *, void *, size_t);
    size_t (*write) (struct file *, const void *, size_t);
};

int do_open(const char *path);
int do_close(int fd);

#endif /* __FILE_H */
