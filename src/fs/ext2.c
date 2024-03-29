#include "ext2.h"
#include "ata.h"
#include "file.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>

#define SB_START_BYTES 1024

#define EXT2_ROOT_INO 2

#define EXT2_NDIR_BLK 12
#define EXT2_IND_BLK EXT2_NDIR_BLK
#define EXT2_DIND_BLK (EXT2_IND_BLK + 1)
#define EXT2_TIND_BLK (EXT2_DIND_BLK + 1)

struct ext2_fs *rootfs;

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

struct ext2_fs *ext2_mount_rootfs()
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

int ext2_block_offsets(struct ext2_fs *fs, unsigned int blk, int offsets[4])
{
    int n = 0;
    const long ptrs_per_block = fs->block_size / 4;

    const long direct_blocks = EXT2_NDIR_BLK;
    const long indirect_blocks = ptrs_per_block;
    const long double_blocks = indirect_blocks * indirect_blocks;
    const long triple_blocks = double_blocks * double_blocks;

    /* Direct block */
    if (blk < direct_blocks) {
        offsets[n++] = blk;
        goto out;
    }
    blk -= direct_blocks;

    /* Single indirect block */
    if ((blk - direct_blocks) < indirect_blocks) {
        offsets[n++] = EXT2_IND_BLK;
        offsets[n++] = blk;
        goto out;
    }
    blk -= indirect_blocks;

    /* Double indirect block */
    if ((blk - indirect_blocks) < double_blocks) {
        offsets[n++] = EXT2_DIND_BLK;
        offsets[n++] = blk / ptrs_per_block;
        offsets[n++] = blk % ptrs_per_block;
        goto out;
    }
    blk -= double_blocks;

    /* Triple indirect block */
    if ((blk - double_blocks) < triple_blocks) {
        offsets[n++] = EXT2_TIND_BLK;
        offsets[n++] = blk / (ptrs_per_block * ptrs_per_block);
        offsets[n++] = blk / ptrs_per_block;
        offsets[n++] = blk % ptrs_per_block;
        goto out;
    }

    panic("block too big: %d", blk);

out:
    return n;
}

void ext2_get_block(struct ext2_fs *fs, struct ext2_ino *in, void *buf, unsigned int blk)
{
    if (blk > in->i_blocks)
        panic("get block at index greater than file size");

    int offsets[4];
    int n = ext2_block_offsets(fs, blk, offsets);

    uint32_t *ptrs = buf;
    read_blk(fs, ptrs, in->i_block[offsets[0]], fs->block_size);
    for (int i = 1; i < n; i++) {
        read_blk(fs, buf, ptrs[offsets[i]], fs->block_size);
    }
}

void ext2_get_blocks(struct ext2_fs *fs, struct ext2_ino *in, char *buf, uint32_t n)
{
    for (int i = 0; i < n; i++) {
        ext2_get_block(fs, in, buf + (i * fs->block_size), i);
    }
}

void *ext2_read_file(struct ext2_fs *fs, int ino)
{
    struct ext2_ino *inode_buf = kcalloc(1, sizeof(struct ext2_ino));
    ext2_get_inode(fs, inode_buf, ino);

    uint32_t nblk = (inode_buf->i_size + fs->block_size - 1) / fs->block_size;
    char *buf = kmalloc(nblk * fs->block_size);
    ext2_get_blocks(fs, inode_buf, buf, nblk);
    memset(buf + inode_buf->i_size, 0, (nblk * fs->block_size) - inode_buf->i_size);

    free(inode_buf);
    return buf;
}

int ext2_namei(struct ext2_fs *fs, const char *path)
{
    char comp[256];
    const char *comp_start = path + 1;
    void *dirbuf = kmalloc(fs->block_size);
    int ino = EXT2_ROOT_INO;

    struct ext2_ino *in = kcalloc(1, sizeof(struct ext2_ino));
    memcpy(in, &fs->rooti, sizeof(fs->rooti));

    while (*comp_start != '\0') {
        int l = 0;
        while (comp_start[l] != '/' && comp_start[l] != '\0')
            l++;
        memcpy(comp, comp_start, l);
        comp[l] = '\0';

        comp_start += l;

        if (*comp_start == '/')
            comp_start++;

        ext2_get_block(fs, in, dirbuf, 0);
        unsigned int doff = 0;
        bool found = false;
        while (doff < fs->block_size) {
            struct ext2_dirent *de = (struct ext2_dirent *)(((char *)dirbuf) + doff);
            if (de->inode == 0)
                goto next_dirent;

            if (strncmp(comp, de->name, de->name_len) == 0) {
                ino = de->inode;
                ext2_get_inode(fs, in, ino);
                found = true;
                break;
            }

        next_dirent:
            doff += de->rec_len;
            continue;
        }

        if (!found)
            panic("path %s does not exist", path);
    }

    free(dirbuf);
    free(in);
    return ino;
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

size_t ext2_getdents(struct file *f, struct dirent *buf, size_t count)
{
    void *data = kmalloc(rootfs->block_size);

    struct ext2_ino in;
    ext2_get_inode(rootfs, &in, f->ino);
    read_blk(rootfs, data, in.i_block[0], rootfs->block_size);

    uint32_t doff = 0;
    uint32_t us_doff = 0;

    while (doff < rootfs->block_size) {
        struct ext2_dirent *de = (struct ext2_dirent *)(((char *)data) + doff);
        struct dirent *us_de = (struct dirent *)(((char *)buf) + us_doff);

        if (de->inode == 0)
            continue;

        us_de->ino = de->inode;
        us_de->rec_len = sizeof(struct dirent) + de->name_len + 1; /* \0 not included */
        strlcpy(us_de->name, de->name, de->name_len);

        doff += de->rec_len;
        us_doff += us_de->rec_len;
    }

    free(data);
    return us_doff;
}

struct file_ops ext2_file_operations = {

};

struct file_ops ext2_dir_operations = {
    .getdents = ext2_getdents,
};
