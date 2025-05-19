#ifndef __MEM_H
#define __MEM_H

#include <stdint.h>
#include "utils.h"

#define HEAP_SIZE 0x8000000

#define SPIN_TABLE_START (void*)0x0
#define SPIN_TABLE_END (void*)0x1000

#define PAGE_SIZE (1ULL << 12)
#define EMPTY INT_MAX
#define NEG_ZERO -INT_MAX

// Memory pool from (1 << 4)-byte to (1 << 11)-byte block
#define MIN_POOL_SIZE_EXP 4
#define MAX_POOL_SIZE_EXP 11
#define BITMAP_ARR PAGE_SIZE / (1ULL << MIN_POOL_SIZE_EXP) / 64

#define USABLE_MEM_DTB_PATH "/memory/reg"

struct buddy_node
{
    int idx;
    struct buddy_node *prev;
    struct buddy_node *next;
};

struct dynamic_node
{
    void *addr; // Virtual
    size_t chunk_size;
    int sum;
    uint64_t bitmap[BITMAP_ARR]; 
    int cur_idx;
    struct dynamic_node *next;
};

struct page
{
    uint16_t ref_count;
    uint8_t w_permission;
};

inline static void *p2v_trans_kernel(void *physical_addr)
{
    return physical_addr + (size_t)v_kernel_space;
}

inline static void *v2p_trans_kernel(void *physical_addr)
{
    return physical_addr - (size_t)v_kernel_space;
}

void memcpy(void *dst, void *src, uint32_t size);
void* memset(void *s, char c, size_t n);
void* simple_malloc(size_t size);
void buddy_init();
void* buddy_malloc(uint32_t size /* The unit is PAGE_SIZE */); // Physical address
void buddy_free(void *ptr); // Physical address
void dynamic_allocator_init();
void* malloc(size_t size); // Virtual address
void *calloc(size_t nitems, size_t size); // Virtual address
void free(void *vptr); // Virtual address
void memory_reserve(void *start, void *end);
void reserve_mem_regions();
bool mem_region(void *p, char *name);
void page_ref_init();
bool get_w_permission(void *ptr); // Physical address
void set_w_permission(void *ptr, bool w); // Physical address
uint16_t get_ref_count(void *ptr); // Physical address
void ref_page(void *ptr); // Physical address
void deref_page(void *ptr); // Physical address

#endif