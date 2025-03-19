#include "utils.h"
#include "uart.h"
#include "string.h"
#include "mem.h"
#include "mailbox.h"
#include "boot.h"
#include "ramdisk.h"
#include "device_tree.h"
#include "interrupt.h"
#include "printf.h"

extern void *dtb_addr;
extern void exec_prog();

void mem_alloc()
{
    char sz[STRLEN];
    size_t size;
    void *data;

    printf("Size: ");
    uart_read_string(sz, STRLEN);

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0)
    {
        printf("Invalid number\r\n");
        return;
    }

    data = simple_malloc(size);

    if (data != NULL)
        printf("Allocator test: The data allocated from the heap is: 0x%x\r\n", (uintptr_t)data);
}

void page_alloc()
{
    char *sz = strtok(NULL, "");
    size_t size;
    void *data;

    if (sz == NULL)
    {
        printf("Invalid argument\r\n");
        return;
    }

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0)
    {
        printf("Invalid number\r\n");
        return;
    }

    data = buddy_malloc(size);

    if (data != NULL)
        printf("Allocator test: The pages allocated are from 0x%x\r\n", (uintptr_t)data);
}

void page_free()
{
    char *str = strtok(NULL, "");
    void *ptr;

    if (str == NULL)
    {
        printf("Invalid argument\r\n");
        return;
    }

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        ptr = (void*)(uintptr_t)hstr2u32(str + 2, strlen(str + 2));
    else    
        ptr = (void*)(uintptr_t)hstr2u32(str, strlen(str));
    
    buddy_free(ptr);
}

int main(void *_dtb_addr)
{
    char cmd[STRLEN], *arg0;

    uart_init();
    clear_read_fifo();
    clear_write_fifo();
    enable_uart_int();

    task_queue_init();

    core_timer_enable();

    buddy_init();
    allocator_init();

    dtb_addr = _dtb_addr;
    fdt_traverse(initramfs_callback);
    
    while (true)
    {
        printf("# ");
        uart_read_string(cmd, STRLEN);
        arg0 = strtok(cmd, " ");
        if (!strcmp("help", arg0))
            printf("help\t: print this help menu\r\n"
                    "hello\t: print Hello World!\r\n"
                    "mailbox\t: print hardware's information\r\n"
                    "reboot\t: reboot the device\r\n"
                    "ls\t: list all the files in ramdisk\r\n"
                    "cat\t: show the content of file1\r\n"
                    "memAlloc: allocate data from the heap\r\n"
                    "ldProg\t: execute the specified program in the ramdisk\r\n"
                    "setTimeout <msg> <time>: print <msg> after <time> seconds\r\n"
                    "pageAlloc <num>\t: allocate <num> pages\r\n"
                    "pageFree <ptr>\t: free pages from <ptr>\r\n");
        else if (!strcmp("hello", arg0))
            printf("Hello World!\r\n");
        else if (!strcmp("mailbox", arg0))
            mailbox_info();
        else if (!strcmp("reboot", arg0))
            reset(1);
        else if (!strcmp("ls", arg0))
            ls();
        else if (!strcmp("cat", arg0))
        {
            if (cat(strtok(NULL, "")) < 0)
                printf("File not found\r\n");
        }
        else if (!strcmp("memAlloc", arg0))
            mem_alloc();
        else if (!strcmp("ldProg", arg0))
        {
            load_prog();
            exec_prog();
        }
        else if (!strcmp("setTimeout", arg0))
        {
            if (set_timeout())
                printf("Invalid argument\r\n");
        }
        else if (!strcmp("pageAlloc", arg0))
            page_alloc();
        else if (!strcmp("pageFree", arg0))
            page_free();
        else
            printf("Invalid command\r\n");
    }
    return 0;
}