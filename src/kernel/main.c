#include "utils.h"
#include "uart.h"
#include "string.h"
#include "mem.h"
#include "mailbox.h"
#include "boot.h"
#include "ramdisk.h"
#include "device_tree.h"
#include "interrupt.h"

extern void *dtb_addr;
extern void exec_prog();
extern void core_timer_enable();

void mem_alloc()
{
    char sz[STRLEN];
    size_t size;
    void *data;

    uart_write_string("Size: ");
    uart_read(sz, STRLEN, STRING_MODE);

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0)
    {
        uart_write_string("Invalid number\r\n");
        return;
    }

    data = simple_malloc(size);

    if (data != NULL)
    {
        uart_write_string("Allocator test: The data allocated from the heap is: ");
        uart_write_hex((uintptr_t)data, sizeof(uint64_t));
        uart_write_newline();
    }
}

int main(void *_dtb_addr)
{
    char cmd[STRLEN];

    uart_init();
    clear_read_fifo();
    clear_write_fifo();
    enable_uart_int();

    core_timer_enable();

    dtb_addr = _dtb_addr;
    fdt_traverse(initramfs_callback);
    
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
                    "memAlloc: allocate data from the heap\r\n"
                    "ldProg\t: execute the specified program in the ramdisk\r\n"
                    "setTimeout <msg> <time>: print <msg> after <time> seconds\r\n");
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
        else if (!strcmp("ldProg", cmd))
        {
            load_prog();
            exec_prog();
        }
        else if (!strcmp(strtok(cmd, " "), "setTimeout"))
        {
            if (set_timeout())
            {
                uart_write_string("Invalid argument\r\n");
            }
        }
        else
            uart_write_string("Invalid command\r\n");
    }
    return 0;
}