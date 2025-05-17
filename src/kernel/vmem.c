#include "vmem.h"
#include "mem.h"

#define IDX(page_idx, level) (((page_idx) & (0x1ffULL << 9 * (level))) >> 9 * (level))

void finer_granu_paging()
{
    void *pud_addr = p2v_trans_kernel((void*)PUD_ADDR); // Virtual addr
    void *pmd_addr = malloc(PAGE_SIZE); // Virtual addr
    size_t pmd_granu = 0x200000; // 2MB

    for (int i = 0; i < NUM_PT_ENTRIES; i++)
    {
        size_t mair_idx = (pmd_granu * i < MMIO_START) ? MAIR_IDX_NORMAL_NOCACHE << 2 : MAIR_IDX_DEVICE_nGnRnE << 2;
        
        *((size_t*)pmd_addr + i) = pmd_granu * i | PD_ACCESS | mair_idx | PD_BLOCK;
    }
    
    *(size_t*)pud_addr = (size_t)v2p_trans_kernel(pmd_addr) | PD_TABLE;
}

void *v2p_trans(void *virtual_addr)
{
    size_t *entry_ptr, entry, table_addr, idx;
    size_t idx_mask = 0xff8000000000, offset_mask = 0x7fffffffff; // The index is at bit 47~39 and shifts left 9 bits each time, and the reset is offset

    if (((size_t)virtual_addr >> 48) == 0xffff) // Kernel space
        asm volatile("mrs %0, ttbr1_el1" : "=r"(entry));
    else  // User space
        asm volatile("mrs %0, ttbr0_el1" : "=r"(entry));

    for (int i = 0; i < NUM_PT_LEVEL; i++) // Find at most 4 levels
    {
        table_addr = entry & TABLE_ADDR_MASK;
        idx = ((size_t)virtual_addr & idx_mask) >> (39 - i * 9);
        entry_ptr = (size_t*)(table_addr | (idx << 3));

        entry = *(size_t*)p2v_trans_kernel(entry_ptr);

        if (!(entry & 0b1))
            return NULL;

        if (!(entry & 0b10) || i == 3) // Page or block found
            break;
        else // Next table
        {
            idx_mask >>= 9;
            offset_mask >>= 9;
        }
    }

    return (void*)((entry & TABLE_ADDR_MASK) | ((size_t)virtual_addr & offset_mask));
}

void fill_page_table(void *pgd_addr, size_t s_page_idx, size_t e_page_idx, size_t start_pa, size_t flags)
{
    size_t *table_addr[NUM_PT_LEVEL]; // 0 -> PTE addr, 1 -> PMD addr, 2 -> PUD addr, 3 -> PGD addr
    int idx = 0;

    table_addr[PGD] = pgd_addr;
    for (int i = PUD; i >= PTE; i--)
    {
        size_t addr_entry = *(table_addr[i + 1] + IDX(s_page_idx, i + 1));
        if (addr_entry & 0b1) // Valid entry
            table_addr[i] = p2v_trans_kernel((void*)(addr_entry & TABLE_ADDR_MASK));
        else
        {
            table_addr[i] = malloc(PAGE_SIZE);
            memset(table_addr[i], 0, PAGE_SIZE);
            *((size_t*)table_addr[i + 1] + IDX(s_page_idx, i + 1)) = (size_t)v2p_trans_kernel(table_addr[i]) | PD_TABLE;
        }
    }
    
    for (; s_page_idx < e_page_idx; s_page_idx++, idx++)
    {
        size_t pte_entry;

        pte_entry = (PAGE_SIZE * idx + start_pa) | PD_USER_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE | flags;
        *((size_t*)table_addr[0] + IDX(s_page_idx, 0)) = pte_entry;

        if (s_page_idx + 1 == e_page_idx)
            break;

        for (int i = PUD; i >= PTE; i--)
        {
            if (IDX(s_page_idx, i) != 0 && IDX(s_page_idx + 1, i) == 0)
            {    
                table_addr[i] = malloc(PAGE_SIZE);
                memset(table_addr[i], 0, PAGE_SIZE);
                *((size_t*)table_addr[i + 1] + IDX(s_page_idx + 1, i + 1)) = (size_t)v2p_trans_kernel(table_addr[i]) | PD_TABLE;
            }
        }
    }
}

void map_code_and_stack(struct pcb_t *pcb)
{
    void *pgd_addr; // Virtual
    void *code_addr = pcb->code->code;
    size_t s_page_idx, e_page_idx, num_pages;
    void *stack_top = pcb->stack[0];

    pgd_addr = malloc(PAGE_SIZE);
    memset(pgd_addr, 0, PAGE_SIZE);
    pcb->ttbr = v2p_trans_kernel(pgd_addr);
    
    // Map code to 0x0
    s_page_idx = (size_t)USR_CODE_START >> 12;
    num_pages = (pcb->code->size - 1) / PAGE_SIZE + 1;
    e_page_idx = s_page_idx + num_pages;
    fill_page_table(pgd_addr, s_page_idx, e_page_idx, (size_t)v2p_trans_kernel(code_addr), PD_USER_ACCESS | PD_ACCESS);
    
    s_page_idx = (size_t)USR_STACK_START >> 12;
    e_page_idx = (size_t)USR_STACK_END >> 12;
    fill_page_table(pgd_addr, s_page_idx, e_page_idx, (size_t)v2p_trans_kernel(stack_top - STACK_SIZE), PD_NX_EL0 | PD_USER_ACCESS | PD_ACCESS);
}

void free_page_table(void *table_addr, enum page_table_lv addr_type)
{
    void *v_table_addr = p2v_trans_kernel(table_addr);

    if (addr_type != PTE)
    {
        for (int i = 0; i < NUM_PT_ENTRIES; i++)
        {
            if (*((size_t*)v_table_addr + i) & PD_TABLE)
                free_page_table((void*)(*((size_t*)v_table_addr + i) & TABLE_ADDR_MASK), addr_type - 1);
        }
    }

    free(v_table_addr);
}

void* mmap(void* addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    void *ttbr;
    size_t num_pages = len / PAGE_SIZE + (len % PAGE_SIZE > 0);
    size_t s_page_idx, e_page_idx;
    size_t continuous_pages = 0;
    size_t pte_flags = 0;

    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr));

    for (void *i = addr - (size_t)addr % PAGE_SIZE; continuous_pages < num_pages; i += PAGE_SIZE)
    {
        if (v2p_trans(i))
            continuous_pages = 0;
        else
        {
            if (!continuous_pages)
                addr = i;
            continuous_pages++;
        }
    }

    s_page_idx = (size_t)addr / PAGE_SIZE;
    e_page_idx = s_page_idx + num_pages;


    // if (flags & MAP_POPULATE)
        pte_flags |= PD_ACCESS;
    if (!(prot & PROT_NONE))
        pte_flags |= PD_USER_ACCESS;
    if (!(prot & PROT_EXEC))
        pte_flags |= PD_NX_EL0;
    if (!(prot & PROT_WRITE))
        pte_flags |= PD_RDONLY;

    for (size_t pages = num_pages; s_page_idx < e_page_idx && pages > 0; pages >>= 1)
    {
        while (e_page_idx - s_page_idx >= pages)
        {
            void *address = malloc(pages * PAGE_SIZE);

            if (!address)
                break;

            fill_page_table(p2v_trans_kernel(ttbr), s_page_idx, s_page_idx + pages, (size_t)v2p_trans_kernel(address), pte_flags);
            s_page_idx += pages;
        }
    }

    if (s_page_idx < e_page_idx)
    {
        // FIXME: should release allocated pages here
        err("No enough space\r\n");
        return NULL;
    }

    return addr;
}