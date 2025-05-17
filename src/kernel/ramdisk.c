#include "ramdisk.h"
#include "uart.h"
#include "utils.h"
#include "device_tree.h"
#include "string.h"
#include "printf.h"
#include "mem.h"

void *ramdisk_saddr;
void *ramdisk_eaddr;
static int dtb_str_idx = 0;

void ls()
{
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
            continue;
        }

        if (!strcmp("TRAILER!!!", record->payload))
            break;
        
        if (!strcmp(".", record->payload) || strncmp("./", record->payload, 2))
            printf("%s\r\n", record->payload);
        else
            printf("%s\r\n", record->payload + 2);

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

int cat(char *filename)
{
    void *addr = ramdisk_saddr;

    if (filename == NULL)
        return -1;
        
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (addr >= ramdisk_eaddr)
        {
            err("No TRAILER record\r\n");
            return -1;
        }

        if (strncmp("070701", record->hdr.c_magic, strlen("070701")))
        {
            err("Invalid .cpio record\r\n");
            continue;
        }

        if (!strcmp("TRAILER!!!", record->payload))
            return -1;

        if ((!strncmp("./", record->payload, 2) && !strcmp(filename, record->payload + 2)) || (strncmp("./", record->payload, 2) && !strcmp(filename, record->payload)) )
        {
            int offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);

            printf("%s\r\n", record->payload + offset);

            return 0;
        }

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

static bool initramfs_process_node(void *p, char *name, char *path, void **addr_ptr)
{
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

bool initramfs_start(void *p, char *name)
{
    char path[] = INITRD_START_DTB_PATH;

    return initramfs_process_node(p, name, path, &ramdisk_saddr);
}

bool initramfs_end(void *p, char *name)
{
    char path[] = INITRD_END_DTB_PATH;

    return initramfs_process_node(p, name, path, &ramdisk_eaddr);
}

void* load_prog(char *filename, size_t *prog_size)
{
    void *prog_addr;
    void *addr = ramdisk_saddr;
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (addr >= ramdisk_eaddr)
        {
            err("No TRAILER record\r\n");
            return NULL;
        }

        if (strncmp("070701", record->hdr.c_magic, strlen("070701")))
        {
            err("Invalid .cpio record\r\n");
            continue;
        }

        if (!strcmp("TRAILER!!!", record->payload))
            return NULL;

        if ((!strncmp("./", record->payload, 2) && !strcmp(filename, record->payload + 2)) || (strncmp("./", record->payload, 2) && !strcmp(filename, record->payload)) )
        {
            int offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);
            prog_addr = malloc((file_size / PAGE_SIZE + (file_size % PAGE_SIZE > 0)) * PAGE_SIZE);
            *prog_size = file_size;

            for (int i = 0; i < file_size; i++)
                *((char*)prog_addr + i) = *(record->payload + offset + i);

            return prog_addr;
        }

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}