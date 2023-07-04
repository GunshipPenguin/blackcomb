#include "ext2.h"
#include "file.h"
#include "kmalloc.h"
#include "sched.h"
#include "util.h"
#include "ext2.h"

int do_open(const char *path)
{
    struct file *f = kcalloc(1, sizeof(struct file));
    f->ino = ext2_namei(rootfs, path);
    int i;

    for (i = 0; i < TASK_FILE_MAX; i++) {
        if (current->fdtab[i] == NULL) {
            current->fdtab[i] = f;
            break;
        }
    }

    if (i >= TASK_FILE_MAX)
        panic("out of file descriptors");

    struct ext2_ino *inode_buf = kcalloc(1, sizeof(struct ext2_ino));
    ext2_get_inode(rootfs, inode_buf, f->ino);

    if (inode_buf->i_mode & EXT2_S_IFREG)
        f->ops = &ext2_file_operations;
    else if (inode_buf->i_mode & EXT2_S_IFDIR)
        f->ops = &ext2_dir_operations;
    else
        panic("unsupported file mode %x", inode_buf->i_mode);

    free(inode_buf);

    return i;
}

int do_close(int fd)
{
    if (current->fdtab[fd] == NULL)
        panic("bad fd");

    free(current->fdtab[fd]);
    current->fdtab[fd] = NULL;

    return 0;
}
