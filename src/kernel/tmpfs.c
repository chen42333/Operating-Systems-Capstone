#include "tmpfs.h"
#include "mem.h"

static bool vfs_tree_child(void *ptr, void *data) {
    struct vnode *node = ptr;

    if (node->hidden)
        return false;

    return strcmp(node->name, (char*)data) == 0;
}

long tmpfs_write(struct file *file, const void *buf, size_t len) {
    size_t write_sz = (file->f_pos + len > MAX_FILE_SZ) ? MAX_FILE_SZ - file->f_pos : len;

    if (file->f_pos + write_sz > file->vnode->file_size)
        file->vnode->file_size = file->f_pos + write_sz;

    memcpy(file->vnode->internal + file->f_pos, buf, write_sz);
    file->f_pos += write_sz;

    return write_sz;
}

long tmpfs_read(struct file *file, void *buf, size_t len) {
    void *s = file->vnode->internal + file->f_pos;
    size_t read_sz = MIN(len, file->vnode->file_size - file->f_pos);

    memcpy(buf, s, read_sz);
    file->f_pos += read_sz;

    return read_sz;
}

int tmpfs_open(struct vnode *file_node, int flags, struct file **target) {
    struct file *f;

    if (!(f = malloc(sizeof(struct file))))
        return -1;
        
    f->vnode = file_node;
    f->flags = flags;
    f->f_ops = file_node->f_ops;
    f->f_pos = 0;
    f->ref_count = 1;
    
    *target = f;

    return 0;
}

int tmpfs_close(struct file *file) {
    if (--file->ref_count == 0)
        free(file);
    return 0;
}

long tmpfs_lseek64(struct file *file, long offset, int whence) {
    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0 || offset > file->vnode->file_size)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos = offset;
            break;
        case SEEK_CUR:
            if (file->f_pos + offset < 0 || file->f_pos + offset > file->vnode->file_size)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos += offset;
            break;
        case SEEK_END:
            if (offset > 0 || file->vnode->file_size + offset < 0)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos = file->vnode->file_size + offset;
            break;
        default:
            err("Invalid whence\r\n");
            return -1;
            break;
    }
    return 0;
}

int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    *target = (struct vnode*)list_find(&dir_node->children, vfs_tree_child, (void*)component_name);
    
    return 0;
}

int tmpfs_init_vnode(struct vnode *dir_node, struct vnode *node, file_type type, const char *component_name) {
    if (init_vnode(dir_node, node, type, component_name) || 
        (type == FILE && !(node->internal = malloc(MAX_FILE_SZ))))
        return -1;
        
    return 0;
}

int tmpfs_create(struct vnode *dir_node, struct vnode **target, const char *component_name, file_type type) {
    *target = malloc(sizeof(struct vnode));

    return tmpfs_init_vnode(dir_node, *target, type, component_name);
}

int tmpfs_mkdir(struct vnode *dir_node, struct vnode **target, const char *component_name) {
    *target = malloc(sizeof(struct vnode));

    return tmpfs_init_vnode(dir_node, *target, DIR, component_name);
}

int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount, struct vnode *dir_node, const char *component) {
    if (!(mount->root->v_ops = malloc(sizeof(struct vnode_operations))))
        return -1;
    if (!(mount->root->f_ops = malloc(sizeof(struct file_operations))))
    {    
        free(mount->root->v_ops);
        return -1;
    }

    if (tmpfs_init_vnode(NULL, mount->root, DIR, component))
        return -1;
    
    if (dir_node)
        list_push(&dir_node->children, mount->root);
    mount->root->parent = dir_node;

    mount->root->v_ops->create = tmpfs_create;
    mount->root->v_ops->lookup = tmpfs_lookup;
    mount->root->v_ops->mkdir = tmpfs_mkdir;

    mount->root->f_ops->write = tmpfs_write;
    mount->root->f_ops->read = tmpfs_read;
    mount->root->f_ops->open = tmpfs_open;
    mount->root->f_ops->close = tmpfs_close;
    mount->root->f_ops->lseek64 = tmpfs_lseek64;
    mount->root->f_ops->ioctl = NULL;

    mount->root->mount = mount;

    return 0;
}

void tmpfs_init() {
    struct filesystem *tmpfs = malloc(sizeof(struct filesystem));

    if (!tmpfs)
        return;
    tmpfs->name = "tmpfs";
    tmpfs->setup_mount = &tmpfs_setup_mount;

    register_filesystem(tmpfs);
    vfs_mount("/", tmpfs->name, 0);
}