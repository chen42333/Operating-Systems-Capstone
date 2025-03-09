#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "boot.h"
#include "ramdisk.h"
#include "device_tree.h"

extern void *dtb_addr;
extern void exec_prog();

void mem_alloc()
{
    char sz[STRLEN];
    void *data;

    uart_write_string("Size (hex): ", IO_ASYNC);
    uart_read(sz, STRLEN, STRING_MODE, IO_SYNC);

    data = simple_malloc(hstr2u32(sz, strlen(sz)));
    if (data != NULL)
    {
        uart_write_string("Allocator test: The data allocated from the heap is: ", IO_ASYNC);
        uart_write_hex((uintptr_t)data, sizeof(uint64_t), IO_ASYNC);
        uart_write_string("\r\n", IO_ASYNC);
    }
}

int main()
{
    char cmd[STRLEN];

    asm volatile ("mov %0, x28" : "=r"(dtb_addr));
    uart_init();
    enable_uart_int();
    fdt_traverse(initramfs_callback);

    while (true)
    {
        uart_write_string("# ", IO_ASYNC);
        uart_read(cmd, STRLEN, STRING_MODE, IO_SYNC);
        if (!strcmp("help", cmd))
            uart_write_string("help\t: print this help menu\r\n"
                    "hello\t: print Hello World!\r\n"
                    "mailbox\t: print hardware's information\r\n"
                    "reboot\t: reboot the device\r\n"
                    "ls\t: list all the files in ramdisk\r\n"
                    "cat\t: show the content of file1\r\n"
                    "memAlloc: allocate data from the heap\r\n"
                    "ldProg\t: execute the specified program in the ramdisk\r\n", IO_ASYNC);
        else if (!strcmp("hello", cmd))
            uart_write_string("Hello World!\r\n", IO_ASYNC);
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
        else if (!strcmp("ldProg", cmd))
        {
            load_prog();
            exec_prog();
        }
        else
            uart_write_string("Invalid command\r\n", IO_ASYNC);
    }
    return 0;
}