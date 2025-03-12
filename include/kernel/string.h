#ifndef __STRING_H
#define __STRING_H

#include <stdint.h>
#include "utils.h"
#define STRLEN 256

int strcmp(const char *str1, const char *str2);
uint32_t hstr2u32(char *hstr, int size);
uint32_t str2u32(char *str, int size);
void strcpy(char *dst, char *src);
char* strtok(char *str, char *delim);

inline static uint32_t strlen(char* str)
{
    uint32_t i;
    for (i = 0; str[i] != '\0'; i++) ;
    return i;
}

#endif