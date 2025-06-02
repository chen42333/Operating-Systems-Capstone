#ifndef __DEV_H
#define __DEV_H

#include "vfs.h"
#include "file.h"

#define DEVDIR "/dev"
#define DEV_UART "/dev/uart"
#define NUM_DEV sizeof(dev_list) / sizeof(device)

typedef uint32_t mode_t;
typedef uint32_t dev_t;
typedef struct device 
{
    char *pathname;
    dev_t dev_number;
    void (*driver)(struct vnode *node);
} device;

int mknod(const char *pathname, mode_t mode, dev_t dev);
void device_file_init();

#endif