#include "ext2.h"
#include "ata.h"
#include "kmalloc.h"
#include "string.h"
#include "util.h"

#include <stdint.h>

struct ext2_sb {
    uint32_t s_inodes_count;
    uint32_t s_blocks_count;
    uint32_t s_r_blocks_count;
    uint32_t s_free_blocks_count;
    uint32_t s_free_inodes_count;
    uint32_t s_first_data_block;
    uint32_t s_log_block_size;
    uint32_t s_log_frag_size;
    uint32_t s_blocks_per_group;
    uint32_t s_frags_per_group;
    uint32_t s_inodes_per_group;
    uint32_t s_mtime;
    uint32_t s_wtime;
    uint16_t s_mnt_count;
    uint16_t s_max_mnt_count;
    uint16_t s_magic;
    uint16_t s_state;
    uint16_t s_errors;
    uint16_t s_minor_rev_level;
    uint32_t s_lastcheck;
    uint32_t s_checkinterval;
    uint32_t s_creator_os;
    uint32_t s_rev_level;
    uint16_t s_def_resuid;
    uint16_t s_def_resgid;

    /* EXT2 dynamic rev stuff - Unimplemented */
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    char s_uuid[16];
    char s_volume_name[16];
    char s_last_mounted[64];
    uint32_t s_algo_bitmap;

    /* Performance hints - Unimplemented */
    uint8_t s_prealloc_blocks;
    uint8_t s_prealloc_dir_blocks;
    uint16_t pad1;

    /* Journaling - Unimplemented */
    char s_journal_uuid[16];
    uint32_t s_journal_inum;
    uint32_t s_journal_dev;
    uint32_t s_last_orphan;

    /* Directory indexing support - Unimplemented */
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t pad2[3];

    uint32_t s_default_mount_options;
    uint32_t s_first_meta_bg;
};

struct ext2_fs *ext2_mount()
{
    struct ext2_fs *fs = kmalloc(sizeof(struct ext2_fs));
    if (!fs)
        panic("out of memory");

    struct ext2_sb sb;
    memset(&sb, 0, sizeof(sb));
    ata_read_bytes(&sb, 1024, 1024);

    fs->block_size = 1024 << sb.s_log_block_size;

    return fs;
}

void ext2_read_file(const char *path, char *buf, uint64_t off)
{

}
