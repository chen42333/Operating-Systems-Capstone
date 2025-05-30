#include "tmpfs.h"
#include "mem.h"

static bool vfs_tree_child(void *ptr, void *data)
{
    struct vnode *node = ptr;
    struct tmpfs_internal *info = node->internal;

    if (node->hidden)
        return false;

    return strcmp(info->name, (char*)data) == 0;
}

static int tmpfs_write(struct file* file, const void* buf, size_t len)
{
    struct tmpfs_internal *info = file->vnode->internal;
    size_t write_sz = (file->f_pos + len > MAX_FILE_SZ) ? MAX_FILE_SZ - file->f_pos : len;

    if (file->f_pos + write_sz > info->size)
        info->size = file->f_pos + write_sz;

    memcpy(info->content + file->f_pos, buf, write_sz);
    file->f_pos += write_sz;

    return write_sz;
}

static int tmpfs_read(struct file* file, void* buf, size_t len)
{
    void *s = ((struct tmpfs_internal*)file->vnode->internal)->content + file->f_pos;
    struct tmpfs_internal *info = file->vnode->internal;
    size_t read_sz = (len < info->size - file->f_pos) ? len : info->size - file->f_pos;

    memcpy(buf, s, read_sz);
    file->f_pos += read_sz;

    return read_sz;
}
static int tmpfs_open(struct vnode* file_node, int flags, struct file** target)
{
    struct file *f;

    if (!(f = malloc(sizeof(struct file))))
        return -1;
        
    f->vnode = file_node;
    f->flags = flags;
    f->f_ops = file_node->f_ops;
    f->f_pos = 0;
    
    *target = f;

    return 0;
}

static int tmpfs_close(struct file* file)
{
    free(file);
    return 0;
}
static long tmpfs_lseek64(struct file* file, long offset, int whence)
{
    struct tmpfs_internal *info = file->vnode->internal;

    switch (whence)
    {
        case SEEK_SET:
            if (offset < 0 || offset > info->size)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos = offset;
            break;
        case SEEK_CUR:
            if (file->f_pos + offset < 0 || file->f_pos + offset > info->size)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos += offset;
            break;
        case SEEK_END:
            if (offset > 0 || info->size + offset < 0)
            {
                err("Invalid offset\r\n");
                return -1;
            }
            file->f_pos = info->size + offset;
            break;
        default:
            err("Invalid whence\r\n");
            return -1;
            break;
    }
    return 0;
}

static int tmpfs_lookup(struct vnode *dir_node, struct vnode **target, const char *component_name)
{
    *target = (struct vnode*)list_find(&dir_node->children, vfs_tree_child, (void*)component_name);
    
    return 0;
}

static int tmpfs_init_vnode(struct vnode* dir_node, struct vnode *node, file_type type, const char* component_name)
{
    struct tmpfs_internal *info;

    if (!node)
    {
        err("Failed to allocate vnode\r\n");
        return -1;
    }
    
    if (dir_node)
    {
        node->mount = dir_node->mount;
        node->f_ops = dir_node->f_ops;
        node->v_ops = dir_node->v_ops;
    }
    node->hidden = false;
    memset(&node->children, 0, sizeof(struct list));
    
    if (!(node->internal = malloc(sizeof(struct tmpfs_internal))))
    {
        err("Failed to allocate FS internal\r\n");
        free(node);
        return -1;
    }

    info = node->internal;
    strcpy(info->name, component_name);
    info->parent = dir_node;
    info->type = type;
    info->size = 0;
    if (type == FILE && !(info->content = malloc(MAX_FILE_SZ)))
    {
        free(info);
        return -1;
    }
        
    return 0;
}

static int tmpfs_create(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    *target = malloc(sizeof(struct vnode));

    return tmpfs_init_vnode(dir_node, *target, FILE, component_name);
}

static int tmpfs_mkdir(struct vnode* dir_node, struct vnode** target, const char* component_name)
{
    *target = malloc(sizeof(struct vnode));

    return tmpfs_init_vnode(dir_node, *target, DIR, component_name);
}

static int tmpfs_setup_mount(struct filesystem *fs, struct mount *mount, struct vnode *dir_node)
{
    if (!(mount->root->v_ops = malloc(sizeof(struct vnode_operations))))
        return -1;
    if (!(mount->root->f_ops = malloc(sizeof(struct file_operations))))
    {    
        free(mount->root->v_ops);
        return -1;
    }

    if (tmpfs_init_vnode(dir_node, mount->root, DIR, ""))
        return -1;

    mount->root->v_ops->create = tmpfs_create;
    mount->root->v_ops->lookup = tmpfs_lookup;
    mount->root->v_ops->mkdir = tmpfs_mkdir;

    mount->root->f_ops->write = tmpfs_write;
    mount->root->f_ops->read = tmpfs_read;
    mount->root->f_ops->open = tmpfs_open;
    mount->root->f_ops->close = tmpfs_close;
    mount->root->f_ops->lseek64 = tmpfs_lseek64;

    mount->root->mount = mount;

    return 0;
}

void tmpfs_init()
{
    struct filesystem *tmpfs = malloc(sizeof(struct filesystem));

    if (!tmpfs)
        return;
    tmpfs->name = "tmpfs";
    tmpfs->setup_mount = &tmpfs_setup_mount;

    register_filesystem(tmpfs);
    vfs_mount("/", tmpfs->name);
}