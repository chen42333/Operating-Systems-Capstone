#include "vmem.h"
#include "mem.h"

void finer_granu_paging()
{
    void *pud_addr = p2v_trans_kernel((void*)PUD_ADDR); // Virtual addr
    void *pmd_addr = malloc(PAGE_SIZE); // Virtual addr
    size_t pmd_granu = 0x200000;
    size_t num_entries = 1ULL << 9;

    for (int i = 0; i < num_entries; i++)
    {
        size_t mair_idx = (pmd_granu * i < MMIO_START) ? MAIR_IDX_NORMAL_NOCACHE << 2 : MAIR_IDX_DEVICE_nGnRnE << 2;
        
        *(size_t*)(pmd_addr + i * sizeof(size_t)) = pmd_granu * i | PD_ACCESS | mair_idx | PD_BLOCK;
    }
    
    *(size_t*)pud_addr = (size_t)v2p_trans_kernel(pmd_addr) | PD_TABLE;
}

void *v2p_trans(void *virtual_addr)
{
    size_t *entry_ptr, entry, table_addr, idx;
    size_t idx_mask = 0xff8000000000, offset_mask = 0x7fffffffff;
    size_t table_addr_mask = 0xfffffffff000;

    if (((size_t)virtual_addr >> 48) == 0xffff) // Kernel space
        asm volatile("mrs %0, ttbr1_el1" : "=r"(entry));
    else  // User space
        asm volatile("mrs %0, ttbr0_el1" : "=r"(entry));

    for (int i = 0; i < 4; i++) // Find at most 4 levels
    {
        table_addr = entry & table_addr_mask;
        idx = ((size_t)virtual_addr & idx_mask) >> (39 - i * 9);
        entry_ptr = (size_t*)(table_addr | (idx << 3));

        entry = *(size_t*)p2v_trans_kernel(entry_ptr);

        if (!(entry & 0b1))
        {
            err("Invalid page\r\n");
            return NULL;
        }

        if (!(entry & 0b10) || i == 3) // Page or block found
            break;
        else // Next table
        {
            idx_mask >>= 9;
            offset_mask >>= 9;
        }
    }

    return (void*)((entry & table_addr_mask) | ((size_t)virtual_addr & offset_mask));
}