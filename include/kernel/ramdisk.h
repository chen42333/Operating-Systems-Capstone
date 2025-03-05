#ifndef __RAMDISK_H
#define __RAMDISK_H

#define INITRD_NODE_PATH "/chosen/linux,initrd-start"

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

struct cpio_record
{
    struct cpio_newc_header hdr;
    char payload[];
};

void ls();
void cat();
int initramfs_callback(void *p, char *name);
void load_prog();

#endif