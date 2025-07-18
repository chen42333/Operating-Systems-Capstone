#ifndef __TMPFS_H
#define __TMPFS_H

#include "vfs.h"
#include "string.h"

void tmpfs_init();
long tmpfs_write(struct file *file, const void *buf, size_t len);
long tmpfs_read(struct file *file, void *buf, size_t len);
int tmpfs_open(struct vnode *file_node, int flags, struct file **target);
int tmpfs_close(struct file *file);
long tmpfs_lseek64(struct file *file, long offset, int whence);
int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name);
int tmpfs_init_vnode(struct vnode *dir_node, struct vnode *node, file_type type, const char *component_name);
int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name, file_type type);
int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name);

#endif