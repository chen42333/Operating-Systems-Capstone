#ifndef __MEM_H
#define __MEM_H

#include <stdint.h>
#include "utils.h"

#define HEAP_SIZE 0x20000

#define PAGE_SIZE (1 << 12)
#define NUM_PAGES_EXP 7
#define FREE 1000
#define X -1000

// Memory pool from (1 << 4)-byte to (1 << 11)-byte block
#define MIN_POOL_SIZE_EXP 4
#define MAX_POOL_SIZE_EXP 11
#define BITMAP_ARR PAGE_SIZE / (1 << MIN_POOL_SIZE_EXP) / 64

struct buddy_node
{
    int idx;
    struct buddy_node *next;
};

struct dynamic_node
{
    void *addr;
    size_t chunk_size;
    int sum;
    uint64_t bitmap[BITMAP_ARR]; 
    int cur_idx;
    struct dynamic_node *next;
};

extern char _sbrk[];
extern char _ebrk[];

void memcpy(void *dst, void *src, uint32_t size);
void* memset(void *s, char c, size_t n);
void* simple_malloc(size_t size);
void buddy_init();
void* buddy_malloc(uint32_t size /* The unit is PAGE_SIZE*/);
void buddy_free(void *ptr);
void allocator_init();
void* malloc(size_t size);
void *calloc(size_t nitems, size_t size);
void free(void *ptr);

#endif