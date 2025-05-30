#ifndef __VFS_H
#define __VFS_H

#include "utils.h"
#include "list.h"

#define MAX_PATH_LEN 255
#define MAX_COMPONENT_LEN 15
#define MAX_FILE_SZ 4096

#define O_ACCMODE 00000003
#define O_RDONLY 00000000
#define O_WRONLY 00000001
#define O_RDWR 00000002
#define O_CREAT 00000100
#define O_EXCL 00000200
#define O_NOCTTY 00000400
#define O_TRUNC 00001000
#define O_APPEND 00002000
#define O_NONBLOCK 00004000
#define O_SYNC 00010000
#define FASYNC 00020000
#define O_DIRECT 00040000
#define O_LARGEFILE 00100000
#define O_DIRECTORY 00200000
#define O_NOFOLLOW 00400000
#define O_NOATIME 01000000
#define O_CLOEXEC 02000000
#define O_NDELAY O_NONBLOCK

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef enum file_type { FILE, DIR, DEV, LINK } file_type;

struct mount;
struct file_operations;
struct vnode_operations;
struct vnode;
struct filesystem;
struct file;

struct mount 
{
    struct vnode* root;
    struct filesystem* fs;
};
  
struct file_operations 
{
    int (*write)(struct file* file, const void* buf, size_t len);
    int (*read)(struct file* file, void* buf, size_t len);
    int (*open)(struct vnode* file_node, int flags, struct file **target);
    int (*close)(struct file* file);
    long (*lseek64)(struct file* file, long offset, int whence);
};
  
struct vnode_operations 
{
    int (*lookup)(struct vnode* dir_node, struct vnode **target, const char* component_name);
    int (*create)(struct vnode* dir_node, struct vnode **target, const char* component_name);
    int (*mkdir)(struct vnode* dir_node, struct vnode **target, const char* component_name);
};

struct vnode 
{
    struct mount* mount;
    struct vnode_operations* v_ops;
    struct file_operations* f_ops;
    struct list children;
    bool hidden;
    void* internal; // may store component name
};

struct filesystem 
{
    const char* name;
    int (*setup_mount)(struct filesystem *fs, struct mount *mount, struct vnode *dir_node);
};

// file handle
struct file 
{
    struct vnode* vnode;
    size_t f_pos;  // RW position of this file handle
    struct file_operations* f_ops;
    int flags;
};

extern struct mount *rootfs;

void vfs_init();
int register_filesystem(struct filesystem* fs);
int vfs_mount(const char* target, const char* filesystem);
int vfs_open(const char* pathname, int flags, struct file **target);
int vfs_close(struct file* file);
int vfs_write(struct file* file, const void* buf, size_t len);
int vfs_read(struct file* file, void* buf, size_t len);
int vfs_lseek64(struct file* file, long offset, int whence);
int vfs_mkdir(const char* pathname, struct vnode **target);
int vfs_create(const char* pathname, struct vnode **target);
int vfs_lookup(const char* pathname, struct vnode **target);

#endif