#include "fat32.h"
#include "sd.h"
#include "mem.h"
#include "string.h"
#include "vfs.h"
#include "tmpfs.h"

/* Treat block as sector (they are all 512 bytes here) */

#define SIGNATURE "\x55\xaa"
#define FirstDataSector (fat32_data.BPB_RsvdSecCnt + (fat32_data.BPB_NumFATs * fat32_data.BPB_FATSz32))
#define FirstSectorofCluster(N) (((N - 2) * fat32_data.BPB_SecPerClus) + FirstDataSector)

#define fat32_lookup tmpfs_lookup
#define fat32_open tmpfs_open
#define fat32_close tmpfs_close
#define fat32_lseek64 tmpfs_lseek64

struct {
    uint32_t start_blk_idx;
    uint32_t end_of_file;
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_RsvdSecCnt;
    uint8_t BPB_NumFATs;
    uint32_t BPB_FATSz32;
    uint32_t BPB_RootClus;
} fat32_data;

static uint32_t get_fat_entry(uint32_t clus_idx) {
    uint32_t fat[fat32_data.BPB_BytsPerSec / sizeof(uint32_t)];
    int blk_idx = fat32_data.start_blk_idx + fat32_data.BPB_RsvdSecCnt + clus_idx / (BLK_SZ / sizeof(uint32_t));
    
    readblock(blk_idx, fat);
    return fat[clus_idx % (BLK_SZ / sizeof(uint32_t))];
}

static void set_fat_entry(uint32_t clus_idx, uint32_t val) {
    for (int i = 0; i < fat32_data.BPB_NumFATs; i++) {
        uint32_t fat[fat32_data.BPB_BytsPerSec / sizeof(uint32_t)];
        int blk_idx = fat32_data.start_blk_idx + fat32_data.BPB_RsvdSecCnt + 
                        i * fat32_data.BPB_FATSz32 + clus_idx / (BLK_SZ / sizeof(uint32_t));
        
        readblock(blk_idx, fat);
        fat[clus_idx % (BLK_SZ / sizeof(uint32_t))] = val;
        writeblock(blk_idx, fat);
    }

    // TODO: Set FSInfo sector
}

static uint32_t get_free_clus_idx() {
    uint32_t fat[fat32_data.BPB_BytsPerSec / sizeof(uint32_t)];
    uint32_t clus_idx = fat32_data.BPB_RootClus + 1;

    while (true) {
        int blk_idx = fat32_data.start_blk_idx + fat32_data.BPB_RsvdSecCnt + clus_idx / (BLK_SZ / sizeof(uint32_t));
        
        readblock(blk_idx, fat);
        for (int i = clus_idx % (BLK_SZ / sizeof(uint32_t)); i < (BLK_SZ / sizeof(uint32_t)); i++, clus_idx++) {
            if (fat[i] == FAT_ENTRY_FREE)
                goto out;
        }
    }

out:
    return clus_idx;
}

static void fat32_mark_modified(struct vnode *node) {
    // TODO: set ATTR_ARCHIVE bit in the directory entry of the modified file/dir and the ancestors
}

static void fat32_reg_to_dir_entry(struct vnode *node) {
    uint8_t buf[fat32_data.BPB_BytsPerSec];
    uint32_t clus_idx = *(uint32_t*)node->parent->internal;
    
    for (; ; clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK) {
        for (int sec_idx = 0; sec_idx < fat32_data.BPB_SecPerClus; sec_idx++) {
            readblock(FirstSectorofCluster(clus_idx) + sec_idx + fat32_data.start_blk_idx, buf);

            for (struct s_dir_entry *d = (struct s_dir_entry*)buf; (uint8_t*)d < buf + fat32_data.BPB_BytsPerSec; d++) {
                if (d->DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED 
                    || d->DIR_Name[0] == DIR_ENTRY_UNUSED || d->DIR_Name[0] == DIR_ENTRY_UNUSED_JP) {
                    int i = 0, j = 0;

                    if (d->DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED) {
                        struct s_dir_entry *_d = d + 1;
                        _d->DIR_Name[0] = DIR_ENTRY_LAST_AND_UNUSED;
                    }

                    for (; i < strlen(node->name) && node->name[i] != '.'; i++, j++)
                        d->DIR_Name[j] = node->name[i];
                    for (; j < sizeof(d->DIR_Name); j++)
                        d->DIR_Name[j] = SPACE;
                    i++;
                    j = 0;
                    for (; i < strlen(node->name); i++, j++)
                        d->DIR_Name_Ext[j] = node->name[i];
                    for (; j < sizeof(d->DIR_Name_Ext); j++)
                        d->DIR_Name_Ext[j] = SPACE;
                    
                    d->DIR_Attr = ATTR_ARCHIVE;
                    if (node->type == DIR)
                        d->DIR_Attr |= ATTR_DIRECTORY;
                    d->DIR_FileSize = node->file_size;

                    d->DIR_FstClusHI = *(uint32_t*)node->internal >> 16;
                    d->DIR_FstClusLO = *(uint32_t*)node->internal & 0xffff;

                    // TODO: Set other field

                    return;
                }

                // FIXME: 
                // 1. The current cluster may be full and DIR_ENTRY_LAST_AND_UNUSED entry should be moved to new cluster
                // 2. The filename may be too long and need LFN entries
            }
        }
    }
}

static int fat32_create(struct vnode *dir_node, struct vnode **target, const char *component_name, file_type type) {
    int free_clus_idx;

    *target = malloc(sizeof(struct vnode));
    if (init_vnode(dir_node, *target, type, component_name))
        return -1;

    free_clus_idx = get_free_clus_idx();
    if (!((*target)->internal = malloc(sizeof(uint32_t))))
        return -1;
    *(uint32_t*)(*target)->internal = free_clus_idx;
    set_fat_entry(free_clus_idx, fat32_data.end_of_file);

    fat32_reg_to_dir_entry(*target);
    fat32_mark_modified(*target);
    
    return 0;
}

static int fat32_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    int free_clus_idx;
    uint8_t buf[fat32_data.BPB_BytsPerSec];
    struct s_dir_entry *d;
    
    *target = malloc(sizeof(struct vnode));
    if (init_vnode(dir_node, *target, DIR, component_name))
        return -1;

    free_clus_idx = get_free_clus_idx();
    if (!((*target)->internal = malloc(sizeof(uint32_t))))
        return -1;
    *(uint32_t*)(*target)->internal = free_clus_idx;
    set_fat_entry(free_clus_idx, fat32_data.end_of_file);

    memset(buf, 0, fat32_data.BPB_BytsPerSec);
    d = (struct s_dir_entry*)buf;
    strcpy(".", d->DIR_Name);
    d->DIR_Attr = ATTR_ARCHIVE | ATTR_DIRECTORY;
    d++;
    strcpy("..", d->DIR_Name);
    d->DIR_Attr = ATTR_ARCHIVE | ATTR_DIRECTORY;
    d++;
    d->DIR_Name[0] = DIR_ENTRY_LAST_AND_UNUSED;
    writeblock(FirstSectorofCluster(free_clus_idx), buf);

    fat32_reg_to_dir_entry(*target);
    fat32_mark_modified(*target);

    return 0;
}

static long fat32_write(struct file *file, const void *buf, size_t len) {
    size_t write_sz;
    uint32_t clus_idx = *(uint32_t*)file->vnode->internal;
    uint32_t which_sec = file->f_pos / fat32_data.BPB_BytsPerSec;
    uint32_t which_clus = which_sec / fat32_data.BPB_SecPerClus;

    for (int i = 0; i < which_clus; i++) 
        clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK;

    for (write_sz = 0; write_sz < len; ) {
        uint8_t buffer[BLK_SZ];
        size_t sz;
        int blk_idx = FirstSectorofCluster(clus_idx) + fat32_data.start_blk_idx + which_sec % fat32_data.BPB_SecPerClus;

        sz = MIN(len - write_sz, BLK_SZ - file->f_pos % fat32_data.BPB_BytsPerSec);
        readblock(blk_idx, buffer);
        memcpy(buffer + file->f_pos % fat32_data.BPB_BytsPerSec, buf + write_sz, sz);
        file->f_pos += sz;
        write_sz += sz;
        writeblock(blk_idx, buffer);

        if (file->f_pos > file->vnode->file_size)
            file->vnode->file_size = file->f_pos;

        if (write_sz < len && ++which_sec % fat32_data.BPB_SecPerClus == 0) {   
            uint32_t tmp = clus_idx;

            clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK;
            if (clus_idx == fat32_data.end_of_file) {
                uint32_t free_clus_idx = get_free_clus_idx();

                set_fat_entry(tmp, free_clus_idx);
                set_fat_entry(free_clus_idx, fat32_data.end_of_file);
                clus_idx = free_clus_idx;
            }
        }
    }

    fat32_mark_modified(file->vnode);

    return write_sz;
}

static long fat32_read(struct file *file, void *buf, size_t len) {
    size_t read_sz;
    uint32_t clus_idx = *(uint32_t*)file->vnode->internal;
    uint32_t which_sec = file->f_pos / fat32_data.BPB_BytsPerSec;
    uint32_t which_clus = which_sec / fat32_data.BPB_SecPerClus;

    for (int i = 0; i < which_clus; i++) 
        clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK;

    for (read_sz = 0; read_sz < len; ) {
        uint8_t buffer[BLK_SZ];
        size_t sz;

        sz = MIN(len - read_sz, BLK_SZ - file->f_pos % fat32_data.BPB_BytsPerSec);
        sz = MIN(sz, file->vnode->file_size - file->f_pos);
        readblock(FirstSectorofCluster(clus_idx) + fat32_data.start_blk_idx + which_sec % fat32_data.BPB_SecPerClus, buffer);
        memcpy(buf + read_sz, buffer + file->f_pos % fat32_data.BPB_BytsPerSec, sz);
        file->f_pos += sz;
        read_sz += sz;

        if (file->f_pos >= file->vnode->file_size)
            break;

        if (read_sz < len && ++which_sec % fat32_data.BPB_SecPerClus == 0) {
            clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK;
            if (clus_idx == fat32_data.end_of_file)
                break;
        }
    }

    return read_sz;
}

// From "Microsoft Extensible Firmware Initiative FAT32 File System Specification"
static unsigned char ChkSum (unsigned char *pFcbName) {
    short FcbNameLen;
    unsigned char Sum;
    Sum = 0;
    for (FcbNameLen=11; FcbNameLen!=0; FcbNameLen--) {
        // NOTE: The operation is an unsigned char rotate right
        Sum = ((Sum & 1) ? 0x80 : 0) + (Sum >> 1) + *pFcbName++;
    }
    return (Sum);
}

static void fat32_parse_sd_recursive(struct vnode *parent, uint32_t clus_idx) {
    uint8_t buf[fat32_data.BPB_BytsPerSec];
    uint8_t lfn_name[LFN_NAME_LEN];
    int name_pos = LFN_NAME_LEN - 2;
    uint8_t checksum;

    memset(lfn_name, 0, LFN_NAME_LEN);

    for (; ; clus_idx = get_fat_entry(clus_idx) & FAT_VAL_MASK) {
        for (uint8_t sec_idx = 0; sec_idx < fat32_data.BPB_SecPerClus; sec_idx++) {
            readblock(FirstSectorofCluster(clus_idx) + sec_idx + fat32_data.start_blk_idx, buf);

            for (struct s_dir_entry *d = (struct s_dir_entry*)buf; (uint8_t*)d < buf + fat32_data.BPB_BytsPerSec; d++) {
                uint32_t next_clus_idx;

                if (d->DIR_Name[0] == DIR_ENTRY_LAST_AND_UNUSED)
                    goto out;
                if (d->DIR_Name[0] == DIR_ENTRY_UNUSED || d->DIR_Name[0] == DIR_ENTRY_UNUSED_JP)
                    continue;
                if ((d->DIR_Attr & ~ATTR_ARCHIVE) == ATTR_VOLUME_ID) // The FAT32 volume itself
                    continue;

                if ((d->DIR_Attr & ~ATTR_ARCHIVE) == ATTR_LONG_NAME) { // LFN
                    struct l_dir_entry *ld = (struct l_dir_entry*)d;
                    for (int i = sizeof(ld->LDIR_Name3) - 1; i >= 0; i--) {
                        if (!ld->LDIR_Name3[i] || ld->LDIR_Name3[i] == 0xff)
                            continue;
                        lfn_name[name_pos--] = ld->LDIR_Name3[i];
                    }
                    for (int i = sizeof(ld->LDIR_Name2) - 1; i >= 0; i--) {
                        if (!ld->LDIR_Name2[i] || ld->LDIR_Name2[i] == 0xff)
                            continue;
                        lfn_name[name_pos--] = ld->LDIR_Name2[i];
                    }
                    for (int i = sizeof(ld->LDIR_Name1) - 1; i >= 0; i--) {
                        if (!ld->LDIR_Name1[i] || ld->LDIR_Name1[i] == 0xff)
                            continue;
                        lfn_name[name_pos--] = ld->LDIR_Name1[i];
                    }

                    checksum = ld->LDIR_Chksum;

                } else { // SFN
                    struct vnode *node;
                    char name[LFN_NAME_LEN];
                    int i = 0;

                    next_clus_idx = ((uint32_t)d->DIR_FstClusHI << 16) + d->DIR_FstClusLO;

                    if (name_pos < LFN_NAME_LEN - 2) { // previous LFN entries
                        if (ChkSum((unsigned char*)d->DIR_Name) != checksum) {
                            err("LFN entry corrupted\r\n");
                        }

                        strncpy(name, (char*)&lfn_name[name_pos + 1], sizeof(name) - (name_pos + 1));
                        memset(lfn_name, 0, LFN_NAME_LEN);
                        name_pos = LFN_NAME_LEN - 2;
                    } else {
                        for (i = 0; d->DIR_Name[i] != SPACE && i < sizeof(d->DIR_Name); i++)
                            name[i] = d->DIR_Name[i];
                        name[i++] = '.';
                        for (int j = 0; d->DIR_Name_Ext[j] != SPACE && j < sizeof(d->DIR_Name_Ext) + sizeof(d->DIR_Name_Ext); i++, j++)
                            name[i] = d->DIR_Name_Ext[j];
                        name[i] = '\0';
                    }

                    if ((d->DIR_Attr & ~ATTR_ARCHIVE) == ATTR_DIRECTORY) {
                        node = malloc(sizeof(struct vnode));
                        init_vnode(parent, node, DIR, name);
                        fat32_parse_sd_recursive(node, next_clus_idx);
                    } else {
                        node = malloc(sizeof(struct vnode));
                        init_vnode(parent, node, FILE, name);

                        node->file_size = d->DIR_FileSize;
                        if (!(node->internal = malloc(sizeof(uint32_t))))
                            return;
                        *(uint32_t*)node->internal = next_clus_idx;
                    }
                }
            }
        }
    }
out:
    return;
}

static int fat32_setup_mount(struct filesystem *fs, struct mount *mount, struct vnode *dir_node, const char *component) {
    if (!(mount->root->v_ops = malloc(sizeof(struct vnode_operations))))
        return -1;
    if (!(mount->root->f_ops = malloc(sizeof(struct file_operations)))) {    
        free(mount->root->v_ops);
        return -1;
    }

    if (init_vnode(NULL, mount->root, DIR, component))
        return -1;
    
    if (dir_node)
        list_push(&dir_node->children, mount->root);
    mount->root->parent = dir_node;

    mount->root->v_ops->create = fat32_create;
    mount->root->v_ops->lookup = fat32_lookup;
    mount->root->v_ops->mkdir = fat32_mkdir;

    mount->root->f_ops->write = fat32_write;
    mount->root->f_ops->read = fat32_read;
    mount->root->f_ops->open = fat32_open;
    mount->root->f_ops->close = fat32_close;
    mount->root->f_ops->lseek64 = fat32_lseek64;
    mount->root->f_ops->ioctl = NULL;

    mount->root->mount = mount;

    if (!(mount->root->internal = malloc(sizeof(uint32_t)))) {
        free(mount->root->v_ops);
        free(mount->root->f_ops);
        return -1;
    }
    *(uint32_t*)mount->root->internal = fat32_data.BPB_RootClus;
    fat32_parse_sd_recursive(mount->root, fat32_data.BPB_RootClus);

    return 0;
}

static int parse_fat32_partition() {
    char buf[BLK_SZ];

    readblock(0, buf);
    if (strncmp(buf + BLK_SZ - 2, SIGNATURE, 2)) {
        err("Invalid MBR\r\n");
        return -1;
    }
    for (int i = 0x1be; i < 0x1fe; i += 0x10) {
        if ((buf[i] & 0xff) == 0x80) { // Boot indiactor
            if ((buf[i + 4] & 0xff) != 0xb) { // Partition Type ID
                err("Not MBR partitioned\r\n");
                return -1;
            }

            // Starting LBA Address (Relative Sectors)
            // It's not 4-byte aligned. It may not allow to directly assign
            memcpy(&fat32_data.start_blk_idx, buf + i + 8, 4);
            break;
        }
    }

    return 0;
}

static int parse_fat32_metadata() {
    char buf[BLK_SZ];
    uint32_t *fat;

    readblock(fat32_data.start_blk_idx, buf);

    memcpy(&fat32_data.BPB_BytsPerSec, buf + 11, 2);
    memcpy(&fat32_data.BPB_SecPerClus, buf + 13, 1);
    memcpy(&fat32_data.BPB_RsvdSecCnt, buf + 14, 2);
    memcpy(&fat32_data.BPB_NumFATs, buf + 16, 1);
    memcpy(&fat32_data.BPB_FATSz32, buf + 36, 4);
    memcpy(&fat32_data.BPB_RootClus, buf + 44, 4);

    if (strncmp(buf + fat32_data.BPB_BytsPerSec - 2, SIGNATURE, 2)) {
        err("Invalid FAT32 volume\r\n");
        return -1;
    }

    readblock(fat32_data.start_blk_idx + fat32_data.BPB_RsvdSecCnt, buf);
    fat = (uint32_t*)buf;
    fat32_data.end_of_file = fat[1];

    return 0;
}

void fat32_init() {
    struct vnode *node;
    struct filesystem *fat32;

    if (parse_fat32_partition() || parse_fat32_metadata())
        return;

    fat32 = malloc(sizeof(struct filesystem));
    if (!fat32)
        return;
    fat32->name = "fat32";
    fat32->setup_mount = &fat32_setup_mount;

    register_filesystem(fat32);
    vfs_mkdir(FAT32_MOUNT_POINT, &node);
    vfs_mount(FAT32_MOUNT_POINT, fat32->name, 0);
}