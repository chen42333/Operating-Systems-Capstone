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

void *v2p_trans(void *virtual_addr, void *ttbr0)
{
    size_t *entry_ptr, entry, table_addr, idx;
    size_t idx_mask = 0xff8000000000, offset_mask = 0x7fffffffff; // The index is at bit 47~39 and shifts left 9 bits each time, and the rest is offset

    if (((size_t)virtual_addr >> 48) == 0xffff) // Kernel space
        asm volatile("mrs %0, ttbr1_el1" : "=r"(entry));
    else  // User space
    {
        if (ttbr0)
            entry = (size_t)ttbr0;
        else
            asm volatile("mrs %0, ttbr0_el1" : "=r"(entry));
    }

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
        *((size_t*)table_addr[0] + IDX(s_page_idx, PTE)) = pte_entry;

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

void init_ttbr(struct pcb_t *pcb)
{
    void *pgd_addr = malloc(PAGE_SIZE); // Virtual
    memset(pgd_addr, 0, PAGE_SIZE);
    pcb->ttbr = v2p_trans_kernel(pgd_addr);
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

void page_table_fork(void *dst_table_addr, void *src_table_addr, enum page_table_lv addr_type)
{
    void *v_src_table_addr = p2v_trans_kernel(src_table_addr);
    void *v_dst_table_addr = p2v_trans_kernel(dst_table_addr);

    for (int i = 0; i < NUM_PT_ENTRIES; i++)
    {
        if (addr_type != PTE && *((size_t*)v_src_table_addr + i) & PD_TABLE) // Table
        {
            void *src_table_addr_next = (void*)(*((size_t*)v_src_table_addr + i) & TABLE_ADDR_MASK);
            void *v_dst_table_addr_next = malloc(PAGE_SIZE);
            void *dst_table_addr_next = v2p_trans_kernel(v_dst_table_addr_next);

            *((size_t*)v_dst_table_addr + i) = (size_t)dst_table_addr_next | PD_TABLE;
            page_table_fork(dst_table_addr_next, src_table_addr_next, addr_type - 1);
        } else { // Block of page
            void *page_addr = (void*)(*((size_t*)v_src_table_addr + i) & TABLE_ADDR_MASK);
            
            if (page_addr < usable_memory[1])
            {
                if (!(*((size_t*)v_src_table_addr + i) & PD_RDONLY)) // Has write permission
                    set_w_permission(page_addr, true);
                *((size_t*)v_src_table_addr + i) |= PD_RDONLY; 
            }
            *((size_t*)v_dst_table_addr + i) = *((size_t*)v_src_table_addr + i);
        }
    }
}

void replace_page_entry(void *ttbr0, void *virtual_addr, void *new_addr, bool w_permission)
{
    size_t *entry_ptr, *v_entry_ptr, entry, table_addr, idx;
    size_t idx_mask = 0xff8000000000;// The index is at bit 47~39 and shifts left 9 bits each time

    if (((size_t)virtual_addr >> 48) == 0xffff) // Kernel space
        asm volatile("mrs %0, ttbr1_el1" : "=r"(entry));
    else  // User space
    {
        if (ttbr0)
            entry = (size_t)ttbr0;
        else
            asm volatile("mrs %0, ttbr0_el1" : "=r"(entry));
    }

    for (int i = 0; i < NUM_PT_LEVEL; i++) // Find at most 4 levels
    {
        table_addr = entry & TABLE_ADDR_MASK;
        idx = ((size_t)virtual_addr & idx_mask) >> (39 - i * 9);
        entry_ptr = (size_t*)(table_addr | (idx << 3));
        v_entry_ptr = (size_t*)p2v_trans_kernel(entry_ptr);

        entry = *v_entry_ptr;

        if (!(entry & 0b1))
            return;

        if (!(entry & 0b10) || i == 3) // Page or block found
            break;
        else // Next table
            idx_mask >>= 9;
    }

    if (w_permission)
        *v_entry_ptr &= ~PD_RDONLY;
    else 
        *v_entry_ptr |= PD_RDONLY;

    *v_entry_ptr &= ~TABLE_ADDR_MASK;
    *v_entry_ptr |= (size_t)new_addr & TABLE_ADDR_MASK;
}

void* mmap(void* addr, size_t len, int prot, int flags, int fd, int file_offset)
{
    struct pcb_t *pcb = get_current();
    void *ttbr;
    size_t num_pages = len / PAGE_SIZE + (len % PAGE_SIZE > 0);
    size_t continuous_pages = 0;

    asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr));

    for (void *i = addr - (size_t)addr % PAGE_SIZE; continuous_pages < num_pages; i += PAGE_SIZE)
    {
        if (v2p_trans(i, NULL))
            continuous_pages = 0;
        else
        {
            if (!continuous_pages)
                addr = i;
            continuous_pages++;
        }
    }

    add_section(pcb, HEAP, addr, num_pages * PAGE_SIZE);

    if (flags & MAP_POPULATE)
    {
        size_t s_page_idx, e_page_idx;
        size_t pte_flags = PD_ACCESS;

        s_page_idx = (size_t)addr / PAGE_SIZE;
        e_page_idx = s_page_idx + num_pages;

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
    }

    return addr;
}