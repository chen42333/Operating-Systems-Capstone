#ifndef __VMEM_H
#define __VMEM_H

#include "utils.h"
#include "paging.h"
#include "process.h"

#define MMIO_START 0x3f000000
#define NUM_PT_ENTRIES (1ULL << 9)
#define TABLE_ADDR_MASK 0xfffffffff000 // Bit 47~12

#define PROT_NONE 0x0
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define PROT_EXEC 0x4

# define MAP_ANONYMOUS 0x20
# define MAP_POPULATE 0x8000 

typedef enum page_table_lv {
    PTE = 0,
    PMD = 1,
    PUD = 2,
    PGD = 3,
    NUM_PT_LEVEL = 4
} page_table_lv;

void finer_granu_paging();
void *v2p_trans(void *virtual_addr, void *ttbr0);
void fill_page_table(void *pgd_addr, size_t s_page_idx, size_t e_page_idx, size_t start_pa, size_t flags); // Address type: (v, v, v, p)
void init_ttbr(struct pcb_t *pcb);
void free_page_table(void *table_addr, page_table_lv addr_type); // Physical
void page_table_fork(void *dst_table_addr, void *src_table_addr, page_table_lv addr_type);  // Physical
void replace_page_entry(void *ttbr0, void *virtual_addr, void *new_addr, bool w_permission);
void* mmap(void *addr, size_t len, int prot, int flags, int fd, int file_offset);

#endif