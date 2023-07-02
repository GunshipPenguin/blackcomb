#ifndef __FILE_H
#define __FILE_H

#include <stdint.h>
#include <stddef.h>

struct file {
    int ino;
    struct file_ops *ops;
};

struct file_ops {
    size_t (*read) (struct file *, void *, size_t);
    size_t (*write) (struct file *, const void *, size_t);
};

#endif /* __FILE_H */
