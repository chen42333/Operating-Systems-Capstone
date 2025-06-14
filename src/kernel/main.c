#include "utils.h"
#include "uart.h"
#include "string.h"
#include "mem.h"
#include "mailbox.h"
#include "boot.h"
#include "ramdisk.h"
#include "device_tree.h"
#include "interrupt.h"
#include "process.h"
#include "printf.h"
#include "syscall.h"
#include "vmem.h"
#include "tmpfs.h"
#include "file.h"
#include "dev.h"
#include "sd.h"
#include "fat32.h"

/* Test functions */
void mem_alloc();
void page_alloc();
void page_free();
void _malloc();
void _free();
void foo(void *args);
void fork_test();

struct strtok_ctx *ctx_shell;

int main(void *_dtb_addr) {
    char cmd[STRLEN], *arg0;

    // get_current() will get NULL until initializtion finish (init_pcb)
    set_current(NULL);

    uart_init();
    clear_read_fifo();
    clear_write_fifo();
    enable_uart_int();

    task_queue_init();

    core_timer_enable();

    dtb_addr = p2v_trans_kernel(_dtb_addr);
    dtb_get_len();
    fdt_traverse(initramfs_start);
    fdt_traverse(initramfs_end);
    fdt_traverse(mem_region);
    
    buddy_init();
    dynamic_allocator_init();
    reserve_mem_regions();
    page_ref_init();

    finer_granu_paging();

    vfs_init();
    tmpfs_init();
    initramfs_init();
    device_file_init();

    sd_init();
    fat32_init();

    init_pcb();
    thread_create(idle, NULL);

    while (true) {
        printf("# ");
        uart_read_string(cmd, STRLEN);
        arg0 = strtok_r(cmd, " ", &ctx_shell);
        if (!strcmp("help", arg0))
            printf("help\t: print this help menu\r\n"
                    "hello\t: print Hello World!\r\n"
                    "mailbox\t: print hardware's information\r\n"
                    "reboot\t: reboot the device\r\n"
                    "ls <dir>\t: list all the files under <dir>\r\n"
                    "cat <filename>\t: show the content of <filename>\r\n"
                    "memAlloc <size>\t: allocate <size> bytes data using simple allocator\r\n"
                    "ldProg <filename>: execute <filename> in the ramdisk\r\n"
                    "setTimeout <msg> <time>: print <msg> after <time> seconds\r\n"
                    "pageAlloc <num>\t: allocate <num> pages from pageframe allocator\r\n"
                    "pageFree <ptr>\t: free pages allocated by pageframe allocator from <ptr>\r\n"
                    "malloc <size>\t: allocate <size> bytes data using dynamic allocator\r\n"
                    "free <ptr>\t: free data allocated by dynamic allocator at <ptr>\r\n"
                    "thread\t: test multi-thread\r\n"
                    "forkTest\t: run fork_test (built-in, at EL1)\r\n"
                    "ps\t: check existing process\r\n");
        else if (!strcmp("hello", arg0))
            printf("Hello World!\r\n");
        else if (!strcmp("mailbox", arg0))
            mailbox_info();
        else if (!strcmp("reboot", arg0))
            reset(1);
        else if (!strcmp("ls", arg0))
            ls(strtok_r(NULL, "", &ctx_shell));
        else if (!strcmp("cat", arg0)) {
            if (cat(strtok_r(NULL, "", &ctx_shell)) < 0)
                printf("File not found\r\n");
        }
        else if (!strcmp("memAlloc", arg0))
            mem_alloc();
        else if (!strcmp("ldProg", arg0)) {
            pid_t pid = fork();

            if (pid == 0)
                exec(strtok_r(NULL, "", &ctx_shell), NULL);
            else
                wait(PROC, pid);
        }
        else if (!strcmp("setTimeout", arg0)) {
            if (set_timeout())
                printf("Invalid argument\r\n");
        }
        else if (!strcmp("pageAlloc", arg0))
            page_alloc();
        else if (!strcmp("pageFree", arg0))
            page_free();
        else if (!strcmp("malloc", arg0))
            _malloc();
        else if (!strcmp("free", arg0))
            _free();
        else if (!strcmp("thread", arg0)) {
            for(int i = 0; i < 5; i++)
                thread_create(foo, NULL);
            // idle();
        }
        else if (!strcmp("forkTest", arg0))
            fork_test();
        else if (!strcmp("ps", arg0))
            ps();
        else
            printf("Invalid command\r\n");

        free(ctx_shell);
    }

    free_init_pcb();
    return 0;
}

void mem_alloc() {
    char *sz = strtok_r(NULL, "", &ctx_shell);
    size_t size;
    void *data;

    if (sz == NULL) {
        printf("Invalid argument\r\n");
        return;
    }

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0) {
        printf("Invalid number\r\n");
        return;
    }

    data = simple_malloc(size);

    if (data != NULL)
        printf("Allocator test: The data allocated from the heap is: 0x%x\r\n", (uintptr_t)data);
}

void page_alloc() {
    char *sz = strtok_r(NULL, "", &ctx_shell);
    size_t size;
    void *data;

    if (sz == NULL) {
        printf("Invalid argument\r\n");
        return;
    }

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0) {
        printf("Invalid number\r\n");
        return;
    }

    data = p2v_trans_kernel(buddy_malloc(size));

    if (data != NULL)
        printf("Allocator test: The pages allocated are from 0x%x\r\n", (uintptr_t)data);
}

void page_free() {
    char *str = strtok_r(NULL, "", &ctx_shell);
    void *ptr;

    if (str == NULL) {
        printf("Invalid argument\r\n");
        return;
    }

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        ptr = (void*)(uintptr_t)hstr2u32(str + 2, strlen(str + 2));
    else    
        ptr = (void*)(uintptr_t)hstr2u32(str, strlen(str));
    
    ptr = v2p_trans(ptr, NULL);
    if (ptr)
        buddy_free(ptr);
    else {
        err("Invalid page\r\n");
    }
}

void _malloc() {
    char *sz = strtok_r(NULL, "", &ctx_shell);
    size_t size;
    void *data;

    if (sz == NULL) {
        printf("Invalid argument\r\n");
        return;
    }

    if (sz[0] == '0' && (sz[1] == 'x' || sz[1] == 'X'))
        size = hstr2u32(sz + 2, strlen(sz + 2));
    else
        size = str2u32(sz, strlen(sz));

    if (size == 0) {
        printf("Invalid number\r\n");
        return;
    }

    data = malloc(size);

    if (data != NULL)
        printf("Allocator test: The pages allocated are from 0x%x\r\n", (uintptr_t)data);
}

void _free() {
    char *str = strtok_r(NULL, "", &ctx_shell);
    void *ptr;

    if (str == NULL) {
        printf("Invalid argument\r\n");
        return;
    }

    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        ptr = (void*)(uintptr_t)hstr2u32(str + 2, strlen(str + 2));
    else    
        ptr = (void*)(uintptr_t)hstr2u32(str, strlen(str));
    
    free(ptr);
}

void foo(void *args) {
    struct pcb_t *pcb = get_current();

    for(int i = 0; i < 10; ++i) {
        printf("PID: %d %d\r\n", pcb->pid, i);
        for (int j = 0; j < 1000000; j++)
            asm volatile("nop");
        schedule();
    }
}

void fork_test() {
    printf("\r\nFork Test, pid %d\r\n", getpid());
    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));
        printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\r\n", getpid(), cnt, &cnt, cur_sp);
        ++cnt;

        if ((ret = fork()) != 0) {
            asm volatile("mov %0, sp" : "=r"(cur_sp));
            printf("first child pid: %d, cnt: %d, ptr: %x, sp : %x\r\n", getpid(), cnt, &cnt, cur_sp);
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                printf("second child pid: %d, cnt: %d, ptr: %x, sp : %x\r\n", getpid(), cnt, &cnt, cur_sp);
                for (int i = 0; i < 1000000; i++)
                    asm volatile("nop");
                ++cnt;
            }
        }
        exit();
    }
    else {
        printf("parent here, pid %d, child %d\r\n", getpid(), ret);
    }
}