#ifndef __STRING_H
#define __STRING_H

#include <stdint.h>
#include "utils.h"
#define STRLEN 256

struct strtok_ctx {
    char strtok_str[STRLEN];
    int strtok_idx;
    int strtok_len;
};

int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t count);
uint32_t hstr2u32(char *hstr, int size);
uint32_t str2u32(char *str, int size);
void strcpy(char *dst, const char *src);
void strncpy(char *dst, const char *src, size_t count);
char* strtok_r(char *str, char *delim, struct strtok_ctx **ctx_ptr);

inline static uint32_t strlen(char *str) {
    uint32_t i;
    for (i = 0; str[i] != '\0'; i++) ;
    return i;
}

#endif