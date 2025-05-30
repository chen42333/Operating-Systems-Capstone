#ifndef __TMPFS_H
#define __TMPFS_H

#include "vfs.h"
#include "string.h"

struct tmpfs_internal
{
    char name[STRLEN];
    struct vnode *parent;
    file_type type;
    size_t size; // for file only
    char *content;
};

void tmpfs_init();

#endif