#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>

#define true 1
#define false 0
#define NULL 0
#define STRLEN 256
#define HEAP_SIZE 0x20000

typedef uint64_t size_t; 

extern char _ebss[];

void err(char *str);
int strcmp(const char *str1, const char *str2);
void set32(void *addr, uint32_t value);
uint32_t get32(void *addr);
void set8(void *addr, char value);
char get8(void *addr);
unsigned int strlen(char* str);
uint32_t hstr2u32(char *str, int size);
void memcpy(void *dst, void *src, uint32_t size);
void* simple_malloc(size_t size);

#endif