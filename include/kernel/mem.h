#ifndef __MEM_H
#define __MEM_H

#include <stdint.h>
#include "utils.h"

#define HEAP_SIZE 0x20000

#define PAGE_SIZE (1 << 12)
#define NUM_PAGES_EXP 7
#define FREE 1000
#define X -1000

extern char _sbrk[];
extern char _ebrk[];

struct node
{
    int idx;
    struct node *next;
};

void memcpy(void *dst, void *src, uint32_t size);
void* simple_malloc(size_t size);
void buddy_init();
void* buddy_malloc(uint32_t size /* The unit is PAGE_SIZE*/);
void buddy_free(void *ptr);

#endif