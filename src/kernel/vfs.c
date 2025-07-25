#include "vfs.h"
#include "string.h"
#include "process.h"
  
struct mount *rootfs;
static struct list fs_list;

static bool same_fs_name(void *ptr, void *data) {
    struct filesystem *fs = ptr;
    char *fs_name = data;

    if (!strcmp(fs->name, fs_name))
        return true;

    return false;
}

int find_parent_vnode(const char *pathname, struct vnode **node, struct vnode **parent_vnode, char *component) {
    disable_int();

    struct mount *mnt;
    struct strtok_ctx *ctx;
    char *comp = strtok_r((char*)pathname, "/", &ctx), *tmp;
    struct vnode *cur, *parent = NULL;
    struct pcb_t *pcb = get_current();
    bool skip = false;

    if (!strcmp("", comp))
        cur = rootfs->root;
    else if (!strcmp("..", comp))
        cur = pcb->cur_dir->parent;
    else if (!strcmp(".", comp))
        cur = pcb->cur_dir;
    else {
        cur = pcb->cur_dir;
        skip = true;
    }

    mnt = cur->mount;
    parent = cur->parent;

    while (skip || ((tmp = strtok_r(NULL, "/", &ctx)) != NULL && strlen(tmp) > 0)) { // Ignore the last '/' (except the first)
        if (skip)
            skip = false;
        else
            comp = tmp;
            
        parent = cur;
        mnt->root->v_ops->lookup(parent, &cur, comp);
        if (!cur) {
            if (strtok_r(NULL, "/", &ctx) != NULL) {
                enable_int();
                err("%s: No such directory\r\n", comp);
                return -1;
            } 
            else
                break;
        }
        mnt = cur->mount;
    }

    *node = cur;
    *parent_vnode = parent;
    strcpy(component, comp);

    free(ctx);

    enable_int();

    return 0;
}

void vfs_init() {
    memset(&fs_list, 0, sizeof(struct list));
}

int register_filesystem(struct filesystem *fs) {
    if (list_find(&fs_list, same_fs_name, (void*)fs->name)) {
        err("Invalid file system name (already exists)\r\n");
        return -1;
    }

    list_push(&fs_list, fs);
    return 0;
}

int vfs_mount(const char *target, const char *filesystem, unsigned flags) {
    struct filesystem *fs = list_find(&fs_list, same_fs_name, (void*)filesystem);
    struct mount *mnt;
    struct vnode *node, *parent_vnode = NULL;
    char component[STRLEN];
    int ret;

    if (!fs) {
        err("File system not registered\r\n");
        return -1;
    }

    if (!(mnt = malloc(sizeof(struct mount))))
        return -1;

    mnt->fs = fs;
    mnt->flags = flags;

    if (!(mnt->root = malloc(sizeof(struct vnode)))) {
        free(mnt);
        return -1;
    }

    if (!strcmp("/", target)) {
        rootfs = mnt;
        strcpy(component, "");
    } else { // The mount point should already exist
        if (find_parent_vnode(target, &node, &parent_vnode, component) < 0) {
            free(mnt->root);
            free(mnt);
            return -1;
        }
        if (!node || node->type != DIR) {
            err("%s: No such directory\r\n", component);
            free(mnt->root);
            free(mnt);
            return -1;
        }

        node->hidden = true; // Hide the original vnode until unmount
    }

    disable_int();
    ret = fs->setup_mount(fs, mnt, parent_vnode, component);
    enable_int();

    return ret;
}

int vfs_open(const char *pathname, int flags, struct file **target) {
    struct vnode *node;
    int ret;

    if (vfs_lookup(pathname, &node) < 0)
        return -1;

    if (!node) {
        if (flags & O_CREAT) // Create the node and then find again
            vfs_create(pathname, &node, FILE);
        else {
            err("File not found\r\n");
            return -1;
        }
    }

    disable_int();
    ret = node->f_ops->open(node, flags, target);
    enable_int();

    return ret;
}

int vfs_close(struct file *file) {
    int ret;

    disable_int();
    ret = file->f_ops->close(file);
    enable_int();

    return ret;
}

long vfs_write(struct file *file, const void *buf, size_t len) {
    if (file->vnode->mount->flags & MS_RDONLY || file->flags & O_RDONLY) {
        err("Permission denied\r\n");
        return -1;
    }

    return file->f_ops->write(file, buf, len);
}

long vfs_read(struct file *file, void *buf, size_t len) {
    if (file->flags & O_WRONLY) {
        err("Permission denied\r\n");
        return -1;
    }

    return file->f_ops->read(file, buf, len);
}

int vfs_lseek64(struct file *file, long offset, int whence) {
    int ret;

    disable_int();
    ret = file->f_ops->lseek64(file, offset, whence);
    enable_int();

    return ret;
}

int vfs_ioctl(struct file *file, unsigned long request, void *data) {
    int ret;

    disable_int();
    ret = file->f_ops->ioctl(file, request, data);
    enable_int();

    return ret;
}

int vfs_mkdir(const char *pathname, struct vnode**target) {
    disable_int();

    struct mount *mnt;
    struct vnode *node, *parent_vnode;
    char component[STRLEN];

    if (find_parent_vnode(pathname, &node, &parent_vnode, component) < 0) {
        enable_int();
        return -1;
    }
    if (node) {
        enable_int();
        err("Directoty exists\r\n");
        return -1;
    }
    mnt = parent_vnode->mount;

    if (mnt->root->v_ops->mkdir(parent_vnode, target, component) < 0) {
        enable_int();
        return -1;
    }

    enable_int();

    return 0;
}

int vfs_create(const char *pathname, struct vnode **target, file_type type) {
    disable_int();

    struct mount *mnt;
    struct vnode *node, *parent_vnode;
    char component[STRLEN];

    if (find_parent_vnode(pathname, &node, &parent_vnode, component) < 0) {
        enable_int();
        return -1;
    }
    if (node) {
        enable_int();
        err("File exists\r\n");
        return -1;
    }
    mnt = parent_vnode->mount;

    if (mnt->root->v_ops->create(parent_vnode, target, component, type) < 0) {
        enable_int();
        return -1;
    }

    enable_int();

    return 0;
}

int vfs_lookup(const char *pathname, struct vnode **target) {
    disable_int();

    struct vnode *node, *parent_vnode;
    char component[STRLEN];

    if (find_parent_vnode(pathname, &node, &parent_vnode, component) < 0) {
        enable_int();
        return -1;
    }

    *target = node;

    enable_int();

    return 0;
}

int init_vnode(struct vnode *dir_node, struct vnode *node, file_type type, const char *component_name) {
    disable_int();

    if (!node) {
        enable_int();
        err("Failed to allocate vnode\r\n");
        return -1;
    }
    
    if (dir_node) {
        node->mount = dir_node->mount;
        node->f_ops = dir_node->f_ops;
        node->v_ops = dir_node->v_ops;
        list_push(&dir_node->children, node);
    }
    node->parent = dir_node;
    node->type = type;
    node->hidden = false;
    memset(&node->children, 0, sizeof(struct list));
    strcpy(node->name, component_name);
    node->file_size = 0;

    enable_int();
        
    return 0;
}