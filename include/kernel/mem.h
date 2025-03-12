#ifndef __MEM_H
#define __MEM_H

#include <stdint.h>
#include "utils.h"

#define HEAP_SIZE 0x20000

void memcpy(void *dst, void *src, uint32_t size);
void* simple_malloc(size_t size);

#endif