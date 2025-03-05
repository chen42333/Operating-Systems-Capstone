#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>
#include "uart.h"

#define true 1
#define false 0
#define NULL 0
#define STRLEN 256
#define HEAP_SIZE 0x20000

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

int strcmp(const char *str1, const char *str2);
uint32_t hstr2u32(char *str, int size);
void memcpy(void *dst, void *src, uint32_t size);
void* simple_malloc(size_t size);
uint32_t big2host(uint32_t num);

inline static void err(char *str)
{
    uart_write_string("Error:\t");
    uart_write_string(str);
    return;
}

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

inline static uint32_t strlen(char* str)
{
    uint32_t i;
    for (i = 0; str[i] != '\0'; i++) ;
    return i;
}

#endif