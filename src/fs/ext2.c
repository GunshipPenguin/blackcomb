#include "ext2.h"
#include "ata.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"
#include "util.h"

#include <stdint.h>

#define SB_START_BYTES 1024

#define EXT2_ROOT_INO 2

void read_blk(struct ext2_fs *fs, void *buf, uint32_t boff, uint32_t len)
{
    ata_read_bytes(buf, boff * fs->block_size, len);
}

void ext2_get_inode(struct ext2_fs *fs, struct ext2_ino *buf, int ino)
{
    struct ext2_bgd *bg = &fs->bgdt[ino / fs->sb.s_inodes_per_group];

    int ioff = (ino % fs->sb.s_inodes_per_group) - 1;
    unsigned int off = (bg->bg_inode_table * fs->block_size) + (ioff * sizeof(struct ext2_ino));

    ata_read_bytes(buf, off, sizeof(struct ext2_ino));
}

struct ext2_fs *ext2_mount()
{
    struct ext2_fs *fs = kmalloc(sizeof(struct ext2_fs));
    if (!fs)
        panic("out of memory");

    struct ext2_sb sb;
    memset(&sb, 0, sizeof(sb));
    ata_read_bytes(&sb, SB_START_BYTES, sizeof(sb));
    fs->sb = sb;
    fs->block_size = 1024 << sb.s_log_block_size;

    /* block_size is set so we can use read now */
    int bgdt_elems = (sb.s_blocks_count / sb.s_blocks_per_group);
    fs->bgdt = kmalloc(bgdt_elems * sizeof(struct ext2_bgd));
    read_blk(fs, fs->bgdt, 2, bgdt_elems * sizeof(struct ext2_bgd));

    /* EXT2_GOOD_OLD_REV specifies that the first 11 inodes are reserved */
    struct ext2_ino r_inodes[11];
    read_blk(fs, &r_inodes, fs->bgdt[0].bg_inode_table, sizeof(r_inodes));
    fs->rooti = r_inodes[1];

    return fs;
}

void ext2_get_block(struct ext2_fs *fs, struct ext2_ino *in, char *buf, unsigned int blk)
{
    if (blk > in->i_blocks)
        panic("Can't get block at index greater than file size");

    if (blk > 12)
        panic("Do you really think I've implemented double and triple indirect blocks at this "
              "point?");

    read_blk(fs, buf, in->i_block[blk], fs->block_size);
}

void ext2_get_blocks(struct ext2_fs *fs, struct ext2_ino *in, char *buf, uint32_t n)
{
    for (int i = 0; i < n; i++) {
        ext2_get_block(fs, in, buf + (i * fs->block_size), i);
    }
}

void *ext2_read_file(struct ext2_fs *fs, struct ext2_ino *in)
{
    uint32_t nblk = (in->i_size + fs->block_size - 1) / fs->block_size;
    char *buf = kmalloc(nblk * fs->block_size);
    ext2_get_blocks(fs, in, buf, nblk);
    memset(buf + in->i_size, 0, (nblk * fs->block_size) - in->i_size);

    return buf;
}

void ext2_namei(struct ext2_fs *fs, struct ext2_ino **in, const char *path)
{
    char comp[256];
    const char *comp_start = path + 1;
    void *dirbuf = kmalloc(fs->block_size);

    *in = kmalloc(fs->block_size);
    memcpy(*in, &fs->rooti, sizeof(fs->rooti));

    while (*comp_start != '\0') {
        int l = 0;
        while (comp_start[l] != '/' && comp_start[l] != '\0')
            l++;
        memcpy(comp, comp_start, l);
        comp[l] = '\0';

        comp_start += l;

        ext2_get_block(fs, *in, dirbuf, 0);
        unsigned int doff = 0;
        while (doff < fs->block_size) {
            struct ext2_dirent *de = (struct ext2_dirent *)(((char *)dirbuf) + doff);
            if (de->inode == 0)
                goto next_dirent;

            if (strncmp(comp, de->name, de->name_len) == 0) {
                ext2_get_inode(fs, *in, de->inode);
                break;
            }

        next_dirent:
            doff += de->rec_len;
            continue;
        }
    }
}

void ext2_ls(struct ext2_fs *fs, const char *path)
{
    void *data = kmalloc(fs->block_size);
    read_blk(fs, data, fs->rooti.i_block[0], fs->block_size);

    uint32_t doff = 0;

    while (doff < fs->block_size) {
        struct ext2_dirent *de = (struct ext2_dirent *)(((char *)data) + doff);
        if (de->inode == 0)
            continue;

        struct ext2_ino in;
        ext2_get_inode(fs, &in, de->inode);
        uint32_t im = in.i_mode;
        printf("%c%c%c%c%c%c%c%c%c%c ", '-', im & EXT2_S_IRUSR ? 'r' : '-',
               im & EXT2_S_IWUSR ? 'w' : '-', im & EXT2_S_IXUSR ? 'x' : '-',
               im & EXT2_S_IRGRP ? 'r' : '-', im & EXT2_S_IWGRP ? 'w' : '-',
               im & EXT2_S_IXGRP ? 'x' : '-', im & EXT2_S_IROTH ? 'r' : '-',
               im & EXT2_S_IWOTH ? 'w' : '-', im & EXT2_S_IXOTH ? 'x' : '-');

        for (int i = 0; i < de->name_len; i++)
            printf("%c", de->name[i]);
        printf("\n");

        doff += de->rec_len;
    }
}
