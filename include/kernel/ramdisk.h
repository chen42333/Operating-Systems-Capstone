#ifndef __RAMDISK_H
#define __RAMDISK_H

#include "utils.h"

#define INITRD_START_DTB_PATH "/chosen/linux,initrd-start"
#define INITRD_END_DTB_PATH "/chosen/linux,initrd-end"

#define INITRD_MOUNT_POINT "/initramfs"

struct cpio_newc_header {
    char    c_magic[6];
    char    c_ino[8];
    char    c_mode[8];
    char    c_uid[8];
    char    c_gid[8];
    char    c_nlink[8];
    char    c_mtime[8];
    char    c_filesize[8];
    char    c_devmajor[8];
    char    c_devminor[8];
    char    c_rdevmajor[8];
    char    c_rdevminor[8];
    char    c_namesize[8];
    char    c_check[8];
};

struct cpio_record {
    struct cpio_newc_header hdr;
    char payload[];
};

bool initramfs_start(void *p, char *name);
bool initramfs_end(void *p, char *name);
void initramfs_init();

#endif