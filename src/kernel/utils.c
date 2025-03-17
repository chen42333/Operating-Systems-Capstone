#include "utils.h"

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