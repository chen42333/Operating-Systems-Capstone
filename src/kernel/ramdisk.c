#include "ramdisk.h"
#include "uart.h"
#include "utils.h"
#include "device_tree.h"
#include "string.h"
#include "io.h"

void *ramdisk_addr;
int idx = 0;

void ls()
{
    void *addr = ramdisk_addr;
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (!strcmp("TRAILER!!!", record->payload))
            break;
        
        if (!strcmp(".", record->payload))
            printf("%s\r\n", record->payload);
        else
            printf("%s\r\n", record->payload + 2);

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

int cat(char *filename)
{
    void *addr = ramdisk_addr;

    if (filename == NULL)
        return -1;
        
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (!strcmp("TRAILER!!!", record->payload))
            return -1;

        if (path_size >= strlen("./") + 1 && !strcmp(filename, record->payload + 2))
        {
            int offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);

            printf("%s\r\n", record->payload + offset);

            return 0;
        }

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

int initramfs_callback(void *p, char *name)
{
    struct fdt_node_comp *ptr = (struct fdt_node_comp*)p;
    int i;
    bool last = false;
    char path[] = INITRD_NODE_PATH;

    if (big2host(ptr->token) == FDT_END)
    {
        idx = 0;
        return false;
    }
    else if (big2host(ptr->token) != FDT_BEGIN_NODE && big2host(ptr->token) != FDT_PROP)
        return false;

    if (idx == 0 && path[idx] == '/')
        idx++;
    
    for (i = idx; path[i] != '/'; i++)
    {
        if (path[i] == '\0')
        {
            last = true;
            break;
        }
    }
    path[i] = '\0';

    if (!strcmp(&path[idx], name))
    {
        idx = i + 1;
        if (last)
        {
            uint32_t value = big2host(*(uint32_t*)(ptr->data + sizeof(struct fdt_node_prop)));
            ramdisk_addr = (void*)(uintptr_t)value;
            idx = 0;
            return true;
        }
    }

    return false;
}

void load_prog()
{
    void *addr = ramdisk_addr;
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (!strcmp("TRAILER!!!", record->payload))
            break;

        if (!strcmp("./simple.img", record->payload))
        {
            int offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);

            for (int i = 0; i < file_size; i++)
                *(_sprog + i) = *(record->payload + offset + i);

            break;
        }

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}