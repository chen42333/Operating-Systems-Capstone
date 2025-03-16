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

struct 
{
    void *base;
    void *end;
    int arr[1 << NUM_PAGES_EXP];
    struct node *free_blocks_list[NUM_PAGES_EXP + 1];
} static buddy_data;

static void buddy_delete_free_block(int idx, int list_idx)
{
    struct node *cur = buddy_data.free_blocks_list[list_idx];
    struct node *prev = NULL;

    while (cur != NULL && cur->idx != idx)
    {
        prev = cur;
        cur = cur->next;
    }

    if (cur == NULL)
    {
        err("Node not found\r\n");
        return;
    }

    if (prev == NULL) // The node is the head
        buddy_data.free_blocks_list[list_idx] = cur->next;
    else
        prev->next = cur->next;

    // Update the array
    buddy_data.arr[idx] = -list_idx; // The first stores the negative value of block size (exp)
    for (int i = 1; i < (1 << list_idx); i++)
        buddy_data.arr[idx + i] = X;
}

static void buddy_insert_free_block(int idx, int list_idx)
{
    struct node *ptr = simple_malloc(sizeof(struct node));
    struct node *cur = buddy_data.free_blocks_list[list_idx];
    struct node *prev = NULL;

    ptr->idx = idx;

    while (cur != NULL && cur->idx < idx)
    {
        prev = cur;
        cur = cur->next;
    }
        
    ptr->next = cur;
    if (prev == NULL) // The node is the head
        buddy_data.free_blocks_list[list_idx] = ptr;
    else
        prev->next = ptr;

    // Update the array
    buddy_data.arr[idx] = list_idx;    
    for (int i = 1; i < (1 << list_idx); i++)
        buddy_data.arr[idx + i] = FREE;
}

inline static bool buddy_list_empty(int list_idx)
{
    return buddy_data.free_blocks_list[list_idx] == NULL;
}

void buddy_init()
{
    buddy_data.base = _sbrk;
    buddy_data.end = _ebrk;
    
    for (int i = 0; i <= NUM_PAGES_EXP; i++)
        buddy_data.free_blocks_list[i] = NULL;

    buddy_insert_free_block(0, NUM_PAGES_EXP);
}

void buddy_cut_block(int idx, int block_size_exp, uint32_t required_size)
{
    if (block_size_exp == 0 || (1 << (block_size_exp - 1)) < required_size)
    {
        log("require %u pages, allocate %u pages from index %d\r\n", required_size, 1 << block_size_exp, idx);
        return;
    }
        
    log("cut the block of %u (pages) size from index %d\r\n", 1 << block_size_exp, idx);

    buddy_data.arr[idx]++;
    buddy_insert_free_block(idx + (1 << (block_size_exp - 1)), block_size_exp - 1);
    buddy_cut_block(idx, block_size_exp - 1, required_size);
}

void* buddy_malloc(uint32_t size /* The unit is PAGE_SIZE*/)
{
    int list_idx = 0, idx;

    while ((1 << list_idx) < size || buddy_list_empty(list_idx))
    {
        if (++list_idx > NUM_PAGES_EXP)
        {
            printf("No enough space\r\n");
            return NULL;
        }
    }

    idx = buddy_data.free_blocks_list[list_idx]->idx;

    buddy_delete_free_block(idx, list_idx);
    buddy_cut_block(idx, list_idx, size);

    return idx * PAGE_SIZE + buddy_data.base;
}

void buddy_merge_block(int idx, int block_size_exp)
{
    int buddy_idx, merge_idx;

    if (block_size_exp == NUM_PAGES_EXP)
        return;

    if (idx % (1 << (block_size_exp + 1)) == 0)
        buddy_idx = idx + (1 << block_size_exp);
    else 
        buddy_idx = idx - (1 << block_size_exp);
    
    if (buddy_data.arr[buddy_idx] != block_size_exp)
        return;

    merge_idx = (idx < buddy_idx) ? idx : buddy_idx;

    log("merge blocks of %u (pages) size from index %d and %d, to a block of %u (pages) size\r\n" \
        , 1 << block_size_exp, idx, buddy_idx, 1 << (block_size_exp + 1));

    buddy_delete_free_block(idx, block_size_exp);
    buddy_delete_free_block(buddy_idx, block_size_exp);
    buddy_insert_free_block(merge_idx, block_size_exp + 1);
    buddy_merge_block(merge_idx, block_size_exp + 1);
}

void buddy_free(void *ptr)
{
    int idx, list_idx;

    if (ptr < (void*)_sbrk || ptr >= (void*)_ebrk || (uintptr_t)ptr & (PAGE_SIZE - 1))
    {
        printf("Invalid pointer\r\n");
        return;
    }

    idx = (ptr - buddy_data.base) / PAGE_SIZE;

    if (buddy_data.arr[idx] > 0)
    {
        printf("The page is not in use\r\n");
        return;
    }
    else if (buddy_data.arr[idx] == X)
    {   
        printf("Not start of a block\r\n");
        return;
    }

    list_idx = -buddy_data.arr[idx];

    buddy_insert_free_block(idx, list_idx);
    buddy_merge_block(idx, list_idx);



for (int i = 0; i <= NUM_PAGES_EXP; i++)
{
    struct node *tmp = buddy_data.free_blocks_list[i];
    printf("List of size %u:\r\n", 1 << i);
    while (tmp != NULL)
    {
        printf("\tidx %d\r\n", tmp->idx);
        tmp = tmp->next;
    }
}
}
