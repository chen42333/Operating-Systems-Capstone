#include "utils.h"
#include "uart.h"

void *heap_ptr = _ebss;

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

unsigned int strlen(char* str)
{
    unsigned int i;
    for (i = 0; str[i] != '\0'; i++) ;
    return i;
}

uint32_t hstr2u32(char *hstr, int size)
{
    uint32_t ret = 0;

    for (int i = 0; i < size; i++)    
    {
        if (hstr[i] >= '0' && hstr[i] <= '9')
            ret += (hstr[i] - '0');
        else if (hstr[i] >= 'A' && hstr[i] <= 'F')
            ret += (hstr[i] - 'A' + 10);
        else if (hstr[i] >= 'a' && hstr[i] <= 'f')
            ret += (hstr[i] - 'a' + 10);

        if (i < size - 1)
            ret <<= 4;
    }

    return ret;
}

void memcpy(void *dst, void *src, uint32_t size)
{
    volatile char *d = dst, *s = src;

    for (int i = 0; i < size; i++)
        *d++ = *s++;
}

void* simple_malloc(size_t size)
{
    void *ret = heap_ptr;
    if ((size_t)heap_ptr + size <= (size_t)&_ebss + HEAP_SIZE)
    {
        heap_ptr += size;
        return ret;
    }

    uart_write_string("Allocation failed\r\n");
    
    return NULL;
}