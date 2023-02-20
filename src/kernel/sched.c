#include "sched.h"
#include "exec.h"
#include "ext2.h"
#include "kmalloc.h"
#include "string.h"

struct task_struct *current;
struct task_struct *tasklist;

void start_init(struct ext2_fs *fs)
{
    struct ext2_ino *in;
    ext2_namei(fs, &in, "/init");

    tasklist = task_from_elf(fs, in);
}
