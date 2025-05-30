#include "vfs.h"
#include "string.h"
  
struct mount *rootfs;
static struct list fs_list;

static bool same_fs_name(void *ptr, void *data)
{
    struct filesystem* fs = ptr;
    char *fs_name = data;

    if (!strcmp(fs->name, fs_name))
        return true;

    return false;
}

static int find_parent_vnode(const char* pathname, struct vnode **node, struct vnode **parent_vnode, char **component_ptr)
{
    struct mount *mnt = rootfs;
    struct strtok_ctx *ctx;
    char *component = strtok_r((char*)pathname, "/", &ctx);
    struct vnode *cur = mnt->root, *parent = NULL;

    if (strcmp("", component))
    {
        err("Require absolute path\r\n");
        return -1;
    }

    while ((component = strtok_r(NULL, "/", &ctx)) != NULL)
    {
        parent = cur;
        mnt->root->v_ops->lookup(parent, &cur, component);
        if (!cur)
        {
            if (strtok_r(NULL, "/", &ctx) != NULL)
            {
                err("%s: No such directory\r\n", component);
                return -1;
            } 
            else
                break;
        }
        mnt = cur->mount;
    }

    free(ctx);

    *node = cur;
    *parent_vnode = parent;
    *component_ptr = component;

    return 0;
}

void vfs_init()
{
    memset(&fs_list, 0, sizeof(struct list));
}

int register_filesystem(struct filesystem* fs) 
{
// register the file system to the kernel.
// you can also initialize memory pool of the file system here.
    if (list_find(&fs_list, same_fs_name, (void*)fs->name))
    {
        err("Invalid file system name (already exists)\r\n");
        return -1;
    }

    list_push(&fs_list, fs);
    return 0;
}

int vfs_mount(const char* target, const char* filesystem)
{
    struct filesystem* fs = list_find(&fs_list, same_fs_name, (void*)filesystem);
    struct mount *mnt;
    struct vnode *node, *parent_vnode = NULL;

    if (!fs)
    {
        err("File system not registered\r\n");
        return -1;
    }

    if (!(mnt = malloc(sizeof(struct mount))))
        return -1;

    mnt->fs = fs;

    if (!(mnt->root = malloc(sizeof(struct vnode))))
    {
        free(mnt);
        return -1;
    }

    if (!strcmp("/", target))
        rootfs = mnt;
    else // The mount point should already exist
    {
        char *component;

        if (find_parent_vnode(target, &node, &parent_vnode, &component) < 0)
        {
            free(mnt->root);
            free(mnt);
            return -1;
        }
        if (!node)
        {
            err("%s: No such directory\r\n", component);
            free(mnt->root);
            free(mnt);
            return -1;
        }

        node->hidden = true; // Hide the original vnode until unmount
        list_push(&parent_vnode->children, mnt->root);
    }

    return fs->setup_mount(fs, mnt, parent_vnode);
}

int vfs_open(const char* pathname, int flags, struct file **target) 
{
// 1. Lookup pathname
// 2. Create a new file handle for this vnode if found.
// 3. Create a new file if O_CREAT is specified in flags and vnode not found
// lookup error code shows if file exist or not or other error occurs
// 4. Return error code if fails
    struct vnode *node;

    if (vfs_lookup(pathname, &node) < 0)
        return -1;

    if (!node)
    {
        if (flags & O_CREAT) // Create the node and then find again
            vfs_create(pathname, &node);
        else {
            err("File not found\r\n");
            return -1;
        }
    }

    return node->f_ops->open(node, flags, target);
}

int vfs_close(struct file* file) 
{
// 1. release the file handle
// 2. Return error code if fails
    return file->vnode->f_ops->close(file);
}

int vfs_write(struct file* file, const void* buf, size_t len) 
{
// 1. write len byte from buf to the opened file.
// 2. return written size or error code if an error occurs.
    return file->vnode->f_ops->write(file, buf, len);
}

int vfs_read(struct file* file, void* buf, size_t len) 
{
// 1. read min(len, readable size) byte to buf from the opened file.
// 2. block if nothing to read for FIFO type
// 2. return read size or error code if an error occurs.
    return file->vnode->f_ops->read(file, buf, len);
}

int vfs_lseek64(struct file* file, long offset, int whence)
{
    return file->vnode->f_ops->lseek64(file, offset, whence);
}

int vfs_mkdir(const char* pathname, struct vnode**target)
{
    struct mount *mnt;
    struct vnode *node, *parent_vnode;
    char *component;

    if (find_parent_vnode(pathname, &node, &parent_vnode, &component) < 0)
        return -1;
    if (node)
    {
        err("Directoty exists\r\n");
        return -1;
    }
    mnt = parent_vnode->mount;

    if (mnt->root->v_ops->mkdir(parent_vnode, target, component) < 0)
        return -1;
    
    list_push(&parent_vnode->children, *target);

    return 0;
}

int vfs_create(const char* pathname, struct vnode **target)
{
    struct mount *mnt;
    struct vnode *node, *parent_vnode;
    char *component;

    if (find_parent_vnode(pathname, &node, &parent_vnode, &component) < 0)
        return -1;
    if (node)
    {
        err("File exists\r\n");
        return -1;
    }
    mnt = parent_vnode->mount;

    if (mnt->root->v_ops->create(parent_vnode, target, component) < 0)
        return -1;

    list_push(&parent_vnode->children, *target);

    return 0;
}

int vfs_lookup(const char* pathname, struct vnode **target)
{
    struct vnode *node, *parent_vnode;
    char *component;

    if (find_parent_vnode(pathname, &node, &parent_vnode, &component) < 0)
        return -1;

    *target = node;

    return 0;
}
  