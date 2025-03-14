#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>

#define true 1
#define false 0
#define NULL 0
#define INT_MAX 2147483647

typedef uint64_t size_t; 
typedef int bool;

extern char _stext[];
extern char _etext[];
extern char _srodata[];
extern char _erodata[];
extern char _sdata[];
extern char _edata[];
extern char _sbss[];
extern char _ebss[];
extern char _estack[];
extern char _sprog[];

uint32_t big2host(uint32_t num);

inline static void set32(void *addr, uint32_t value) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    *ptr = value;
}

inline static uint32_t get32(void *addr)
{
    volatile uint32_t *ptr = (uint32_t*)addr;
    return *ptr;
}

inline static void set8(void *addr, char value) {
    volatile char *ptr = (char*)addr;
    *ptr = value;
}

inline static char get8(void *addr)
{
    volatile char *ptr = (char*)addr;
    return *ptr;
}

#endif