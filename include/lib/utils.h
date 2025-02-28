#ifndef __UTILS_H
#define __UTILS_H

#include <stdint.h>

#define true 1
#define false 0
#define STRLEN 256

void err(char *str);
int strcmp(const char *str1, const char *str2);
void set32(void *addr, uint32_t value);
uint32_t get32(void *addr);
void set8(void *addr, char value);
char get8(void *addr);
uint32_t str2u32(char *str);
void memcpy(void *dst, void *src, uint32_t size);

#endif