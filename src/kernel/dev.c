#include "dev.h"
#include "uart.h"
#include "process.h"
#include "mailbox.h"
#include "vmem.h"

static long _uart_read(struct file *file, void *buf, size_t len)
{
    return uart_read(buf, len);
}

static long _uart_write(struct file *file, const void *buf, size_t len)
{
    return uart_write(buf, len);
}

static void uart_driver(struct vnode *node)
{
    node->f_ops->read = _uart_read;
    node->f_ops->write = _uart_write;
}

static int fb_ioctl(struct file *file, unsigned long request, void *data)
{
    switch (request)
    {
        case 0:
            memcpy(data, &fb_info, sizeof(struct framebuffer_info));
            break;
        default:
            break;
    }

    return 0;
}

static long fb_write(struct file *file, const void *buf, size_t len)
{
    size_t write_sz = (file->f_pos + len > file->vnode->file_size) ? 
                        file->vnode->file_size - file->f_pos : len;

    memcpy(p2v_trans_kernel(fb_addr) + file->f_pos, buf, write_sz);
    file->f_pos += write_sz;

    return write_sz;
}

static void framebuffer_driver(struct vnode *node)
{
    node->f_ops->read = NULL;
    node->f_ops->write = fb_write;
    node->f_ops->ioctl = fb_ioctl;

    get_fb(&node->file_size);
}

const device dev_list[] = { 
    {"/dev/uart", 0, uart_driver}, 
    {"/dev/framebuffer", 1, framebuffer_driver}};

int mknod(const char *pathname, mode_t mode, dev_t dev)
{
    struct vnode *node;
    struct file_operations *f_ops;

    if ((f_ops = malloc(sizeof(struct file_operations))) < 0)
        return -1;

    if (vfs_create(pathname, &node, DEV) < 0)
    {
        free(f_ops);
        return -1;
    }

    memcpy(f_ops, node->f_ops, sizeof(struct file_operations));
    node->f_ops = f_ops;
    dev_list[dev].driver(node);

    return 0;
}

int ioctl(int fd, unsigned long request, void *data)
{
    struct file *f = get_current()->fd_table[fd];

    return vfs_ioctl(f, request, data);
}

void device_file_init()
{
    struct vnode *node;

    if (vfs_mkdir(DEVDIR, &node) < 0)
        return;

    for (int i = 0; i < NUM_DEV; i++)
    {
        if (mknod(dev_list[i].pathname, 0, dev_list[i].dev_number) < 0)
            return;
    }
}