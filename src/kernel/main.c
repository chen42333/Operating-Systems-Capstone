#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "boot.h"
#include "ramdisk.h"
#include "device_tree.h"

extern void *dtb_addr;

void mem_alloc()
{
    char sz[STRLEN];
    void *data;

    uart_write_string("Size (hex): ");
    uart_read(sz, STRLEN, STRING_MODE);

    data = simple_malloc(hstr2u32(sz, strlen(sz)));
    if (data != NULL)
    {
        uart_write_string("Allocator test: The data allocated from the heap is: ");
        uart_write_hex((uintptr_t)data);
        uart_write_string("\r\n");
    }
}

int main()
{
    char cmd[STRLEN];

    asm volatile ("mov %0, x28" : "=r"(dtb_addr));
    fdt_traverse(initramfs_callback);
    uart_init();

    while (true)
    {
        uart_write_string("# ");
        uart_read(cmd, STRLEN, STRING_MODE);
        if (!strcmp("help", cmd))
            uart_write_string("help\t: print this help menu\r\n"
                    "hello\t: print Hello World!\r\n"
                    "mailbox\t: print hardware's information\r\n"
                    "reboot\t: reboot the device\r\n"
                    "ls\t: list all the files in ramdisk\r\n"
                    "cat\t: show the content of file1\r\n"
                    "memAlloc\t: allocate data from the heap\r\n");
        else if (!strcmp("hello", cmd))
            uart_write_string("Hello World!\r\n");
        else if (!strcmp("mailbox", cmd))
            mailbox_info();
        else if (!strcmp("reboot", cmd))
            reset(1);
        else if (!strcmp("ls", cmd))
            ls();
        else if (!strcmp("cat", cmd))
            cat();
        else if (!strcmp("memAlloc", cmd))
            mem_alloc();
        else
            uart_write_string("Invalid command\r\n");
    }
    return 0;
}