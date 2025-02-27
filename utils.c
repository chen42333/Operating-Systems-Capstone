#include "utils.h"
#include "uart.h"

void err(char *str)
{
    uart_write_string("Error:\t");
    uart_write_string(str);
    return;
}

int strcmp(const char *str1, const char *str2)
{
    for (int i = 0; ; i++)
    {

        if (str1[i] < str2[i])
            return -1;
        if (str1[i] > str2[i])
            return 1;
        if (str1[i] == '\0' && str2[i] == '\0')
            break;
    }
    return 0;
}

void set32(void *addr, uint32_t value) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    *ptr = value;
}

uint32_t get32(void *addr)
{
    volatile uint32_t *ptr = (uint32_t*)addr;
    return *ptr;
}

void set8(void *addr, char value) {
    volatile char *ptr = (char*)addr;
    *ptr = value;
}

char get8(void *addr)
{
    volatile char *ptr = (char*)addr;
    return *ptr;
}