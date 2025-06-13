#include "ramdisk.h"
#include "uart.h"
#include "utils.h"
#include "device_tree.h"
#include "string.h"
#include "printf.h"
#include "mem.h"
#include "vfs.h"
#include "tmpfs.h"

void *ramdisk_saddr;
void *ramdisk_eaddr;
static int dtb_str_idx = 0;

static bool initramfs_process_node(void *p, char *name, char *path, void **addr_ptr) {
    struct fdt_node_comp *ptr = (struct fdt_node_comp*)p;
    int i;
    bool last = false;

    if (big2host(ptr->token) == FDT_END)
    {
        dtb_str_idx = 0;
        return false;
    }
    else if (big2host(ptr->token) != FDT_BEGIN_NODE && big2host(ptr->token) != FDT_PROP)
        return false;

    if (dtb_str_idx == 0 && path[dtb_str_idx] == '/')
        dtb_str_idx++;
    
    for (i = dtb_str_idx; path[i] != '/'; i++)
    {
        if (path[i] == '\0')
        {
            last = true;
            break;
        }
    }
    path[i] = '\0';

    if (!strcmp(&path[dtb_str_idx], name))
    {
        dtb_str_idx = i + 1;
        if (last)
        {
            uint32_t value = big2host(*(uint32_t*)(ptr->data + sizeof(struct fdt_node_prop)));

            *addr_ptr = p2v_trans_kernel((void*)(uintptr_t)value);
            dtb_str_idx = 0;
            return true;
        }
    }

    return false;
}

bool initramfs_start(void *p, char *name) {
    char path[] = INITRD_START_DTB_PATH;

    return initramfs_process_node(p, name, path, &ramdisk_saddr);
}

bool initramfs_end(void *p, char *name) {
    char path[] = INITRD_END_DTB_PATH;

    return initramfs_process_node(p, name, path, &ramdisk_eaddr);
}

static void initramfs_create_recursive(struct mount *mnt, char *path, uint32_t path_size, uint32_t file_size) {
    struct strtok_ctx *ctx;
    char *component = strtok_r(path, "/", &ctx);
    struct vnode *cur, *parent = NULL;
    bool skip = false;

    if (strcmp(".", component))
        skip = true;

    cur = mnt->root;
    parent = cur->parent;

    while (skip || (component = strtok_r(NULL, "/", &ctx)) != NULL)
    {
        if (skip)
            skip = false;
            
        parent = cur;
        mnt->root->v_ops->lookup(parent, &cur, component);
        if (!cur)
        {
            char *tmp;
            if ((tmp = strtok_r(NULL, "/", &ctx)) != NULL) // Directory
            {
                mnt->root->v_ops->mkdir(parent, &cur, component);

                skip = true;
                component = tmp;
            } else { // File
                int content_offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);

                mnt->root->v_ops->create(parent, &cur, component, FILE);
                cur->content = path + content_offset;
                cur->file_size = file_size;
                break;
            }
        }
    }

    free(ctx);
}

static void initramfs_parse_cpio(struct mount *mnt) {
    void *addr = ramdisk_saddr;

    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (addr >= ramdisk_eaddr)
        {
            err("No TRAILER record\r\n");
            break;
        }

        if (strncmp("070701", record->hdr.c_magic, strlen("070701")))
        {
            err("Invalid .cpio record\r\n");
            goto cont;
        }

        if (!strcmp("TRAILER!!!", record->payload))
            break;

        if (!strcmp(".", record->payload) || !strcmp("..", record->payload))
            goto cont;

        initramfs_create_recursive(mnt, record->payload, path_size, file_size);
        
cont:
        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

static int initramfs_setup_mount(struct filesystem *fs, struct mount *mount, struct vnode *dir_node, const char *component) {
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

    mount->root->f_ops->write = NULL;
    mount->root->f_ops->read = tmpfs_read;
    mount->root->f_ops->open = tmpfs_open;
    mount->root->f_ops->close = tmpfs_close;
    mount->root->f_ops->lseek64 = tmpfs_lseek64;
    mount->root->f_ops->ioctl = NULL;

    mount->root->mount = mount;

    initramfs_parse_cpio(mount);

    return 0;
}

void initramfs_init() {
    struct vnode *node;
    struct filesystem *initramfs = malloc(sizeof(struct filesystem));

    if (!initramfs)
        return;
    initramfs->name = "initramfs";
    initramfs->setup_mount = &initramfs_setup_mount;

    register_filesystem(initramfs);
    vfs_mkdir(MOUNT_POINT, &node);
    vfs_mount(MOUNT_POINT, initramfs->name, MS_RDONLY);
}