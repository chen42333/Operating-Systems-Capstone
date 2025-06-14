#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include "printf.h"


#ifndef NULL
#define NULL ((void*)0)
#endif
#define true 1
#define false 0
#define INT_MAX 2147483647

#define MMIO_BASE (void*)(0x3f000000 + v_kernel_space)

#define MAX(x, y) ((x > y) ? x : y)
#define MIN(x, y) ((x < y) ? x : y)

typedef uint64_t size_t;

extern char v_kernel_space[];
extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];
extern char _sbss[];
extern char _ebss[];
extern char _estack[];
extern char _sbrk[];

extern void *dtb_addr;
extern size_t dtb_len;
extern void *ramdisk_saddr;
extern void *ramdisk_eaddr;
extern void *usable_memory[2];

extern void invalidate_tlb();

uint32_t big2host(uint32_t num);

inline static void set32(void *addr, uint32_t value) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    *ptr = value;
}

inline static uint32_t get32(void *addr) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    return *ptr;
}

inline static void set8(void *addr, char value) {
    volatile char *ptr = (char*)addr;
    *ptr = value;
}

inline static char get8(void *addr) {
    volatile char *ptr = (char*)addr;
    return *ptr;
}

#endif