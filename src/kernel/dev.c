#include "dev.h"
#include "uart.h"

static long _uart_read(struct file *file, void *buf, size_t len)
{
    return uart_read(buf, len);
}

static long _uart_write(struct file *file, const void *buf, size_t len)
{
    return uart_write(buf, len);
}

const device dev_list[] = { {"/dev/uart", 0, _uart_read, _uart_write} };

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

    f_ops->read = dev_list[dev].read;
    f_ops->write = dev_list[dev].write;
    f_ops->lseek64 = node->f_ops->lseek64;
    f_ops->open = node->f_ops->open;
    f_ops->close = node->f_ops->close;

    node->f_ops = f_ops;

    return 0;
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