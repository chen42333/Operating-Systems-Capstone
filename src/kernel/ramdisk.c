#include "ramdisk.h"
#include "uart.h"
#include "utils.h"

void ls()
{
    void *addr = (void*)RAMDISK_ADDR;
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (!strcmp("TRAILER!!!", record->payload))
            break;
        
        if (!strcmp(".", record->payload))
            uart_write_string(record->payload);
        else
            uart_write_string(record->payload + 2);
        uart_write_string("\r\n");

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}

void cat()
{
    void *addr = (void*)RAMDISK_ADDR;
    while (true)
    {
        struct cpio_record *record = (struct cpio_record*)addr;
        uint32_t path_size = hstr2u32(record->hdr.c_namesize, 8);
        uint32_t file_size = hstr2u32(record->hdr.c_filesize, 8);

        if (!strcmp("TRAILER!!!", record->payload))
            break;

        if (!strcmp("./file1", record->payload))
        {
            int offset = ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3) - sizeof(struct cpio_newc_header);

            uart_write_string("Filename: ");
            uart_write_string(record->payload + 2);
            uart_write_string("\r\n");
            uart_write_string(record->payload + offset);
            uart_write_string("\r\n");

            break;
        }

        addr += ((sizeof(struct cpio_newc_header) + path_size + 3) & ~3);
        addr += ((file_size + 3) & ~3);
    }
}