#include "mem.h"
#include "uart.h"
#include "printf.h"

void memcpy(void *dst, void *src, uint32_t size)
{
    volatile char *d = dst, *s = src;

    for (int i = 0; i < size; i++)
        *d++ = *s++;
}

static void *heap_ptr = _ebss;

void* simple_malloc(size_t size)
{
    void *ret;
    size_t align_padding = 0;

    // Do the alignment according to the size
    for (int i = (1 << 3); i >= 1; i >>= 1)
    {
        if (size % i == 0)
        {
            if ((size_t)heap_ptr % i != 0)
                align_padding = i - ((size_t)heap_ptr % i);
            break;
        }
    }

    ret = heap_ptr + align_padding;
    
    if ((size_t)heap_ptr + size + align_padding <= (size_t)&_ebss + HEAP_SIZE)
    {
        heap_ptr += (size + align_padding);
        return ret;
    }

    printf("Allocation failed\r\n");
    
    return NULL;
}