#ifndef __VMEM_H
#define __VMEM_H

#include "utils.h"
#include "paging.h"
#include "process.h"

#define MMIO_START 0x3f000000
#define NUM_PT_ENTRIES (1ULL << 9)
#define TABLE_ADDR_MASK 0xfffffffff000 // Bit 47~12

enum page_table_lv {
    PTE = 0,
    PMD = 1,
    PUD = 2,
    PGD = 3,
    NUM_PT_LEVEL = 4
};

void finer_granu_paging();
void *v2p_trans(void *virtual_addr);
void fill_page_table(void *pgd_addr, size_t s_page_idx, size_t e_page_idx, size_t start_pa); // Address type: (p, v, v, p)
void map_code_and_stack(struct pcb_t *pcb);
void free_page_table(void *table_addr, enum page_table_lv addr_type); // Physical

#endif