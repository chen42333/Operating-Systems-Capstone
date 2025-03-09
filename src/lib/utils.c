#include "utils.h"
#include "uart.h"

void *heap_ptr = _ebss;

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

uint32_t hstr2u32(char *hstr, int size)
{
    uint32_t ret = 0;

    for (int i = 0; i < size; i++)    
    {
        ret <<= 4;

        if (hstr[i] >= '0' && hstr[i] <= '9')
            ret += (hstr[i] - '0');
        else if (hstr[i] >= 'A' && hstr[i] <= 'F')
            ret += (hstr[i] - 'A' + 10);
        else if (hstr[i] >= 'a' && hstr[i] <= 'f')
            ret += (hstr[i] - 'a' + 10);
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

    uart_write_string("Allocation failed\r\n", IO_ASYNC);
    
    return NULL;
}

uint32_t big2host(uint32_t num)
{
    uint32_t ret = 0;
    unsigned char *ptr = (unsigned char*)(void*)&num;

    for (int i = 0; i < sizeof(uint32_t); i++)
    {
        ret <<= 8;
        ret += *(ptr++);
    }
    return ret;
}