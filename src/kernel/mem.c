#include "mem.h"
#include "uart.h"
#include "printf.h"
#include "device_tree.h"
#include "string.h"
#include "vmem.h"

// #define TEST_MEM

// simple_malloc(): receive, return, compute in virtual memory addr
// The others: receive, return virtual memory addr, compute in physical memory addr

static int dtb_str_idx = 0;

void memcpy(void *dst, const void *src, uint32_t size) {
    volatile char *d = dst;
    volatile const char *s = src;

    for (int i = 0; i < size; i++)
        *d++ = *s++;
}

void* memset(void *s, char c, size_t n) {
    char *p = (char*)s;

    for (int i = 0; i < n; i++)
        *p++ = c;

    return s;
}

static void *heap_start = _sbrk;
static void *heap_ptr = _sbrk;

void* simple_malloc(size_t size) {
    void *ret;
    size_t align_padding = 0;

    // Do the alignment according to the size
    for (size_t i = (1 << 3); i >= 1; i >>= 1) {
        if (size % i == 0) {
            if ((size_t)heap_ptr % i != 0)
                align_padding = i - ((size_t)heap_ptr % i);
            break;
        }
    }

    ret = heap_ptr + align_padding;
    
    if ((size_t)heap_ptr + size + align_padding <= (size_t)heap_start + HEAP_SIZE) {
        heap_ptr += (size + align_padding);
        return ret;
    }

    printf("Allocation failed\r\n");
    
    return NULL;
}

////////////////////////////////////////
// Data structure of buddy system:
// 1. base, end: the memory range for buddy system to allocate
// 2. arr: each element represents a page, its value means:
//      i.      EMPTY: the page is not head of a block
//      ii.     other pos. int: the size of the block (it's the head of the free block)
//      iii.    other neg. int (or NEG_ZERO): the size of the block (it's the head of the used block)
// 3. free_blocks_list: each element is a linked list of free blocks with 2^idx page size
// 
// buddy_node:
//      1. idx: the index of the page in buddy system
////////////////////////////////////////

struct {
    void *base;
    void *end;
    int num_pages_exp;
    int *arr;
    struct buddy_node **free_blocks_list;
} static buddy_data;

struct buddy_node *buddy_node_arr;
void *usable_memory[2];

inline static bool buddy_list_empty(int list_idx) {
    return buddy_data.free_blocks_list[list_idx] == NULL;
}

static void buddy_delete_free_block(int idx, int list_idx) {
    disable_int();

    struct buddy_node *node_ptr = &buddy_node_arr[idx];

    if (buddy_data.arr[idx] != list_idx) {
        enable_int();
        err("Node not found\r\n");
        return;
    }

    if (node_ptr->prev)
        node_ptr->prev->next = node_ptr->next;
    if (node_ptr->next)
        node_ptr->next->prev = node_ptr->prev;
    
    if (node_ptr == buddy_data.free_blocks_list[list_idx]) // The node is the head
        buddy_data.free_blocks_list[list_idx] = node_ptr->next;

    // Reset the node
    node_ptr->prev = NULL;
    node_ptr->next = NULL;

    // Update the array
    buddy_data.arr[idx] = (list_idx == 0) ? NEG_ZERO : -list_idx; // The first stores the negative value of block size (exp)

    enable_int();

#ifdef TEST_MEM
    log("remove block %d from free list %d\r\n", idx, list_idx);
#endif
}

static void buddy_insert_free_block(int idx, int list_idx) {
    disable_int();

    struct buddy_node *node_ptr = &buddy_node_arr[idx];
    struct buddy_node *head_ptr = buddy_data.free_blocks_list[list_idx];

    if (!buddy_list_empty(list_idx))
        head_ptr->prev = node_ptr;
    node_ptr->next = head_ptr;
    buddy_data.free_blocks_list[list_idx] = node_ptr;

    // Update the array
    buddy_data.arr[idx] = list_idx;  
    
    enable_int();

#ifdef TEST_MEM
    log("add block %d to free list %d\r\n", idx, list_idx);
#endif
}

void buddy_init() {
    size_t int_ptr;

    buddy_data.base = (void*)0x0;

    int_ptr = PAGE_SIZE;
    buddy_data.num_pages_exp = 0;
    while (int_ptr < (size_t)usable_memory[1]) {
        int_ptr <<= 1;
        buddy_data.num_pages_exp++;
    }

    buddy_data.end = (void*)int_ptr;

    buddy_node_arr = simple_malloc((1 << buddy_data.num_pages_exp) * sizeof(struct buddy_node));
    for (int i = 0; i < (1 << buddy_data.num_pages_exp); i++) {
        buddy_node_arr[i].idx = i;
        buddy_node_arr[i].prev = NULL;
        buddy_node_arr[i].next = NULL;
    }

    buddy_data.arr = simple_malloc((1 << buddy_data.num_pages_exp) * sizeof(int));
    for (int i = 0; i < (1 << buddy_data.num_pages_exp); i++)
        buddy_data.arr[i] = EMPTY;
    
    buddy_data.free_blocks_list = simple_malloc((buddy_data.num_pages_exp + 1) * sizeof(struct buddy_node*));
    for (int i = 0; i <= buddy_data.num_pages_exp; i++)
        buddy_data.free_blocks_list[i] = NULL;

    buddy_insert_free_block(0, buddy_data.num_pages_exp);
}

static void buddy_cut_block(int idx, int block_size_exp, uint32_t required_size) {
    disable_int();

    if (block_size_exp == 0 || (1 << (block_size_exp - 1)) < required_size) {
#ifdef TEST_MEM
        log("require %u pages, allocate %u pages from index 0x%x\r\n", required_size, 1 << block_size_exp, idx);
#endif
        enable_int();
        return;
    }

#ifdef TEST_MEM
    log("cut the block of %u (pages) size from index 0x%x\r\n", 1 << block_size_exp, idx);
#endif

    if (++buddy_data.arr[idx] == 0) // The value is negative, so plus 1
        buddy_data.arr[idx] = NEG_ZERO;

    buddy_insert_free_block(idx + (1 << (block_size_exp - 1)), block_size_exp - 1);
    buddy_cut_block(idx, block_size_exp - 1, required_size);

    enable_int();
}

void* buddy_malloc(uint32_t size /* The unit is PAGE_SIZE */) {
    disable_int();

    int list_idx = 0, idx;

    if (size == 0) {
        enable_int();
        return NULL;
    }

    while ((1 << list_idx) < size || buddy_list_empty(list_idx)) {
        if (++list_idx > buddy_data.num_pages_exp) {
            enable_int();
            printf("No enough space\r\n");
            return NULL;
        }
    }

    idx = buddy_data.free_blocks_list[list_idx]->idx;

#ifdef TEST_MEM
    log("allocate block %d\r\n", idx);
#endif

    buddy_delete_free_block(idx, list_idx);
    buddy_cut_block(idx, list_idx, size);

    enable_int();

    return (void*)(idx * PAGE_SIZE); // For kernel space
}

static void buddy_merge_block(int idx, int block_size_exp) {
    disable_int();

    int buddy_idx, big_idx, small_idx;

    if (block_size_exp == buddy_data.num_pages_exp) {
        enable_int();
        return;
    }

    if (idx % (1 << (block_size_exp + 1)) == 0)
        buddy_idx = idx + (1 << block_size_exp);
    else 
        buddy_idx = idx - (1 << block_size_exp);
    
    if (buddy_data.arr[buddy_idx] != block_size_exp) {
        enable_int();
        return;
    }

    small_idx = MIN(idx, buddy_idx);
    big_idx = MAX(idx, buddy_idx);

#ifdef TEST_MEM
    log("merge blocks of %u (pages) size from index 0x%x and 0x%x, to a block of %u (pages) size\r\n" \
        , 1 << block_size_exp, idx, buddy_idx, 1 << (block_size_exp + 1));
#endif

    buddy_delete_free_block(idx, block_size_exp);
    buddy_delete_free_block(buddy_idx, block_size_exp);
    buddy_insert_free_block(small_idx, block_size_exp + 1);
    buddy_data.arr[big_idx] = EMPTY; // Reset manually
    buddy_merge_block(small_idx, block_size_exp + 1);

    enable_int();
}

void buddy_free(void *ptr) {
    disable_int();

    int idx, list_idx;
    
    if (ptr >= buddy_data.end || (uintptr_t)ptr & (PAGE_SIZE - 1)) {
        enable_int();
        printf("Invalid pointer\r\n");
        return;
    }

    idx = (size_t)ptr / PAGE_SIZE;

    if (buddy_data.arr[idx] >= 0) {
        enable_int();
        printf("The page is not in use\r\n");
        return;
    }
    else if (buddy_data.arr[idx] == EMPTY) {   
        enable_int();
        printf("Not start of a block\r\n");
        return;
    }

    list_idx = (buddy_data.arr[idx] == NEG_ZERO) ? 0 : -buddy_data.arr[idx];

#ifdef TEST_MEM
    log("free block %d\r\n", idx);
#endif

    buddy_insert_free_block(idx, list_idx);
    buddy_merge_block(idx, list_idx);

    enable_int();
}

static void buddy_clear_block(int idx, int block_size_exp) {
    int buddy_idx;

    if (idx % (1 << block_size_exp) != 0) {
        err("The index is not aligned with the block size\r\n");
        return;
    }
    
    if (buddy_data.arr[idx] >= block_size_exp)
        return;

    if ((buddy_data.arr[idx] != NEG_ZERO && buddy_data.arr[idx] <= -block_size_exp)
         || (buddy_data.arr[idx] == NEG_ZERO && block_size_exp == 0)) {
        buddy_insert_free_block(idx, block_size_exp);
        return;
    }  

    buddy_idx = idx + (1 << (block_size_exp - 1)); // idx is the start of the block, so idx will be the smaller one
    
    buddy_clear_block(idx, block_size_exp - 1);
    buddy_clear_block(buddy_idx, block_size_exp - 1);

    buddy_delete_free_block(idx, block_size_exp - 1);
    buddy_delete_free_block(buddy_idx, block_size_exp - 1);
    buddy_insert_free_block(idx, block_size_exp);
    buddy_data.arr[buddy_idx] = EMPTY; // Reset manually
}

void memory_reserve(void *start, void *end) {
    void *reserve_start;
    int reserve_len_exp = 0; // The unit is "page"
    int idx, parent_idx;
    size_t size = end - start; // The unit is "byte"

#ifdef TEST_MEM
    log("Reserve 0x%lx-0x%lx\r\n", start, end);
#endif

    // Mark the region as allocated
    for (size_t i = PAGE_SIZE; ; i <<= 1, reserve_len_exp++) { // i stands for size of the smallest block that can cover the whole region
        void *block_start, *block_end;

        if (i < size)
            continue;

        block_start = (void*)((size_t)start & ~(i - 1)); // The largest i-aligned number which is <= start
        block_end = block_start + i;

        if (block_end < end)
            continue;

        // Found the proper block
        reserve_start = block_start;
        break;
    }

    idx = (size_t)reserve_start / PAGE_SIZE;
    parent_idx = idx;
    while (buddy_data.arr[parent_idx] == EMPTY)
        parent_idx -= (parent_idx & -parent_idx); // (parent_idx & -parent_idx) is the largest factor of parent_idx which is power of 2 (the lowest bit 1)
        

    /*
    1. No overlapping
    2. Coverd
    3. Cover the other block(s)

-num_pages_exp   -reserve_len_exp         0         reserve_len_exp     num_pages_exp
      |        2        |        3        |        3        |        1        |
    */

    if (buddy_data.arr[parent_idx] <= -reserve_len_exp && buddy_data.arr[parent_idx] != NEG_ZERO) // The block has been coverd by the other reserved block
        return;
    else if (buddy_data.arr[parent_idx] >= reserve_len_exp) { // No overlapping
        for (int i = buddy_data.arr[parent_idx]; i > reserve_len_exp; i--) {
            int idx1 = parent_idx, idx2 = parent_idx + (1 << (i - 1));

            buddy_delete_free_block(parent_idx, i);
            buddy_insert_free_block(idx1, i - 1);
            buddy_insert_free_block(idx2, i - 1);

            parent_idx = (idx2 > idx) ? idx1 : idx2;
        }

        buddy_delete_free_block(idx, reserve_len_exp);
    }
    else { // The block covers the other block(s), and parent_idx will be equal to idx
        buddy_clear_block(idx, reserve_len_exp); // Clear the overlapped blocks first
        buddy_delete_free_block(idx, reserve_len_exp);
    }

#ifdef TEST_MEM
    log("0x%x-0x%x reserved\r\n", reserve_start, reserve_start + (1ULL << reserve_len_exp) * PAGE_SIZE);
#endif
}

////////////////////////////////////////
// Data structure of dynamic allocator:
// 1. mem_pool: each element is a linked list of allocated pages for 2^(idx + MIN_POOL_SIZE_EXP) bytes request
// 
// dynamic_node:
//      1. addr: the address of the page
//      2. chunk_size: the page is for chunks of what size
// (the following is not used in page allocation)
//      3. sum: add/sub the (idx + 1) of the chunk when malloc/free, sum == 0 means the page can be freed
//      4. bitmap: each bit represents whether a chunk is allocated, 
//                  total needs at most PAGE_SIZE / (1 << MIN_POOL_SIZE_EXP) bits = 4 uint64_t
//      5. cur_idx: the last allocated chunk idx
////////////////////////////////////////

struct dynamic_node *mem_pool[MAX_POOL_SIZE_EXP - MIN_POOL_SIZE_EXP + 1];
static struct dynamic_node *dynamic_node_arr;

void dynamic_allocator_init() {
    for (int i = 0; i <= MAX_POOL_SIZE_EXP - MIN_POOL_SIZE_EXP; i++)
        mem_pool[i] = NULL;

    dynamic_node_arr = simple_malloc((1 << buddy_data.num_pages_exp) * sizeof(struct dynamic_node));
    for (int i = 0; i < (1 << buddy_data.num_pages_exp); i++) {
        dynamic_node_arr[i].addr = (void*)(PAGE_SIZE * i);
        dynamic_node_arr[i].chunk_size = PAGE_SIZE;
        dynamic_node_arr[i].sum = 0;
        dynamic_node_arr[i].next = NULL;
        for (int j = 0; j < BITMAP_ARR; j++)
            dynamic_node_arr[i].bitmap[j] = 0;
        dynamic_node_arr[i].cur_idx = -1;
    }
}

void* malloc(size_t size) {
    disable_int();

    int size_exp = 0;
    struct dynamic_node *prev = NULL, *cur, *node_ptr;
    void *addr;

    if (size == 0) {
        enable_int();
        return NULL;
    }
    if (size >= PAGE_SIZE) {
        void *ret = buddy_malloc((size - 1) / PAGE_SIZE + 1);
        enable_int();
        return p2v_trans_kernel(ret);
    }
    
    for (size_t i = 1; i < size; i <<= 1, size_exp++) ;

    if (size_exp > MAX_POOL_SIZE_EXP) {
        void *ret = buddy_malloc(1);
        enable_int();
        return p2v_trans_kernel(ret);
    }
    if (size_exp < MIN_POOL_SIZE_EXP)
        size_exp = MIN_POOL_SIZE_EXP;

    cur = mem_pool[size_exp - MIN_POOL_SIZE_EXP];

    while (cur != NULL) {
        int num_chunks = PAGE_SIZE / (1 << size_exp);
        for (int i = (cur->cur_idx + 1) % num_chunks; ; i = (i + 1) % num_chunks) {
            int bitmap_arr_idx = i / 64;
            int bitmap_element_idx = i % 64;

            if (cur->bitmap[bitmap_arr_idx] & (1 << bitmap_element_idx)) { // The chunk is allocated
                if (i == cur->cur_idx) // It has looped through the chunks
                    break;
                continue;
            }

            cur->sum += (i + 1);
            cur->bitmap[bitmap_arr_idx] |= (1 << bitmap_element_idx);
            cur->cur_idx = i;

            enable_int();

#ifdef TEST_MEM
            log("allocate %d bytes from 0x%x\r\n", cur->chunk_size, cur->addr + i * cur->chunk_size);
#endif
            return cur->addr + i * cur->chunk_size;
        }

        prev = cur;
        cur = cur->next;
    }

    // There's no free chunk in the memory pool, allocate a new pageframe
    addr = buddy_malloc(1);
    node_ptr = &dynamic_node_arr[(size_t)addr / PAGE_SIZE];

    if (prev == NULL) // The first pageframe in the memory pool
        mem_pool[size_exp - MIN_POOL_SIZE_EXP] = node_ptr;
    else
        prev->next = node_ptr;

    node_ptr->addr = p2v_trans_kernel(addr); // Virtual address
    node_ptr->chunk_size = 1 << size_exp;
    node_ptr->sum += 1;
    node_ptr->bitmap[0] |= 0b1;
    node_ptr->cur_idx = 0;

    enable_int();

#ifdef TEST_MEM
    log("allocate %d bytes from 0x%x\r\n", node_ptr->chunk_size, node_ptr->addr);
#endif
    
    return node_ptr->addr;
}

void* calloc(size_t nitems, size_t size) {
    size_t sz = nitems * size;
    return memset(malloc(sz), 0, sz);
}

void free(void *vptr) {
    disable_int();

    int page_idx;
    int chunk_idx;
    struct dynamic_node *node_ptr;

    void *ptr = v2p_trans_kernel(vptr);
    page_idx = (size_t)ptr / PAGE_SIZE;
    node_ptr = &dynamic_node_arr[page_idx];

    if (ptr >= buddy_data.end) {
        enable_int();
        printf("Invalid pointer\r\n");
        return;
    }

    if (node_ptr->chunk_size == PAGE_SIZE) {
        buddy_free(ptr);
        enable_int();
        return;
    }

    chunk_idx = (vptr - node_ptr->addr) / node_ptr->chunk_size;

    if (!(node_ptr->bitmap[chunk_idx / 64] & (1 << (chunk_idx % 64)))) {
        enable_int();
        printf("The chunk is not in use\r\n");
        return;
    }

    node_ptr->sum -= (chunk_idx + 1);
    node_ptr->bitmap[chunk_idx / 64] &= ~(1 << (chunk_idx % 64));

#ifdef TEST_MEM
    log("free %d bytes from 0x%x\r\n", node_ptr->chunk_size, vptr);
#endif

    if (node_ptr->sum == 0) { // The pageframe can be freed
        struct dynamic_node *prev = NULL, *cur;
        int size_exp = -1;

        for (size_t size = node_ptr->chunk_size; size > 0; size_exp++, size >>= 1) ;

        cur = mem_pool[size_exp - MIN_POOL_SIZE_EXP];
        while (cur != node_ptr) {
           prev = cur;
           cur = cur->next;
        }

        if (prev == NULL)
            mem_pool[size_exp - MIN_POOL_SIZE_EXP] = cur->next;
        else
            prev->next = cur->next;
        
        node_ptr->chunk_size = PAGE_SIZE;
        node_ptr->sum = 0;
        node_ptr->next = NULL;
        for (int i = 0; i < BITMAP_ARR; i++)
            node_ptr->bitmap[i] = 0;

        buddy_free(v2p_trans_kernel(node_ptr->addr));
        node_ptr->addr = (void*)(page_idx * PAGE_SIZE); // Reset to physical address
    }
    enable_int();
}

void reserve_mem_regions() {
    memory_reserve((void*)SPIN_TABLE_START, (void*)SPIN_TABLE_END); // Spin tables for multicore boot
    memory_reserve(v2p_trans_kernel(_stext), v2p_trans_kernel(_estack)); // Kernel image
    memory_reserve((void*)PGD_ADDR, (void*)PUD_ADDR + PAGE_SIZE); // Identity paging
    memory_reserve(v2p_trans_kernel(heap_start), v2p_trans_kernel(heap_start + HEAP_SIZE)); // Startup allocator
    memory_reserve(v2p_trans_kernel(ramdisk_saddr), v2p_trans_kernel(ramdisk_eaddr)); // Initramfs
    memory_reserve(v2p_trans_kernel(dtb_addr), v2p_trans_kernel(dtb_addr + dtb_len)); // Device tree
    // Unusable memory region
    if (usable_memory[0] != buddy_data.base)
        memory_reserve(buddy_data.base, usable_memory[0]);
    if (usable_memory[1] != buddy_data.end)
        memory_reserve(usable_memory[1], buddy_data.end);
}

bool mem_region(void *p, char *name) {
    struct fdt_node_comp *ptr = (struct fdt_node_comp*)p;
    int i;
    bool last = false;
    char path[] = USABLE_MEM_DTB_PATH;

    if (big2host(ptr->token) == FDT_END) {
        dtb_str_idx = 0;
        return false;
    }
    else if (big2host(ptr->token) != FDT_BEGIN_NODE && big2host(ptr->token) != FDT_PROP)
        return false;

    if (dtb_str_idx == 0 && path[dtb_str_idx] == '/')
        dtb_str_idx++;
    
    for (i = dtb_str_idx; path[i] != '/'; i++) {
        if (path[i] == '\0') {
            last = true;
            break;
        }
    }
    path[i] = '\0';

    if (!strcmp(&path[dtb_str_idx], name)) {
        dtb_str_idx = i + 1;
        if (last) {
            uint32_t start = big2host(*(uint32_t*)(ptr->data + sizeof(struct fdt_node_prop)));
            uint32_t end = big2host(*(uint32_t*)(ptr->data + sizeof(struct fdt_node_prop) + sizeof(uint32_t)));

            usable_memory[0] = (void*)(uintptr_t)start;
            usable_memory[1] = (void*)(uintptr_t)end;

            dtb_str_idx = 0;
            return true;
        }
    }

    return false;
}

static struct page *usr_pages;

void page_ref_init() {
    usr_pages = simple_malloc((1 << buddy_data.num_pages_exp) * sizeof(struct page));
    memset(usr_pages, 0, (1 << buddy_data.num_pages_exp) * sizeof(struct page));
}

void set_w_permission(void *ptr, bool w) {
    disable_int();

    size_t idx = (size_t)ptr / PAGE_SIZE;
    if ((size_t)ptr % PAGE_SIZE) {
        printf("Not start of a page\r\n");
        enable_int();
        return;
    }

    usr_pages[idx].w_permission = w;
    
    enable_int();
}

bool get_w_permission(void *ptr) {
    disable_int();

    bool ret = false;
    size_t idx = (size_t)ptr / PAGE_SIZE;
    if ((size_t)ptr % PAGE_SIZE) {
        printf("Not start of a page\r\n");
        enable_int();
        return ret;
    }

    ret = usr_pages[idx].w_permission;
    
    enable_int();

    return ret;
}

uint16_t get_ref_count(void *ptr) {
    disable_int();

    uint16_t ret = ~0;
    size_t idx = (size_t)ptr / PAGE_SIZE;
    if ((size_t)ptr % PAGE_SIZE) {
        printf("Not start of a page\r\n");
        enable_int();
        return ret;
    }

    ret = usr_pages[idx].ref_count;
    
    enable_int();

    return ret;
}

void ref_page(void *ptr) {
    disable_int();

    size_t idx = (size_t)ptr / PAGE_SIZE;
    if ((size_t)ptr % PAGE_SIZE) {
        printf("Not start of a page\r\n");
        enable_int();
        return;
    }
    
    usr_pages[idx].ref_count++;
    enable_int();
}

void deref_page(void *ptr) {
    disable_int();

    size_t idx = (size_t)ptr / PAGE_SIZE;

    if ((size_t)ptr % PAGE_SIZE) {
        printf("Not start of a page\r\n");
        enable_int();
        return;
    }

    if (usr_pages[idx].ref_count > 0 && --usr_pages[idx].ref_count == 0) {
        usr_pages[idx].w_permission = false;
        if (buddy_data.arr[idx] < 0 || buddy_data.arr[idx] == NEG_ZERO) // The page is a start of a used block
            buddy_free(ptr);
    }
    enable_int();
}