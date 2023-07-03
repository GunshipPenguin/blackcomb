#include "ext2.h"
#include "file.h"
#include "kmalloc.h"
#include "sched.h"
#include "util.h"

int do_open(const char *path)
{
    struct file *f = kcalloc(1, sizeof(struct file));
    f->ino = ext2_namei(rootfs, path);

    for (int i = 0; i < TASK_FILE_MAX; i++) {
        if (current->fdtab[i] == NULL) {
            current->fdtab[i] = f;
            return i;
        }
    }

    panic("out of file descriptors");
}

int do_close(int fd)
{
    if (current->fdtab[fd] == NULL)
        panic("bad fd");

    free(current->fdtab[fd]);
    current->fdtab[fd] = NULL;

    return 0;
}
