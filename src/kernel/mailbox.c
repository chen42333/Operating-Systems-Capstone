#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "printf.h"
#include "vmem.h"

int mbox_call(unsigned char channel, uint32_t *mailbox)
{
    uint32_t *p_mbox = v2p_trans((void*)mailbox, NULL);
    uint32_t *v_mbox = p2v_trans_kernel(p_mbox);
    uint32_t data = ((unsigned long)p_mbox & ~0xf) | channel;

    while (get32(MAILBOX_STATUS) & MAILBOX_FULL) ;
    set32(MAILBOX_WRITE, data);
    while (get32(MAILBOX_STATUS) & MAILBOX_EMPTY) ;
    while (get32(MAILBOX_READ) != data) ;

    // Check whether it contains frame buffer allocation request, to translate the buffer addr for user process
    if (v_mbox[1] == REQUEST_SUCCEED)
    {
        int idx = 2;
        
        while (v_mbox[idx] != END_TAG)
        {
            if (v_mbox[idx++] == ALLOC_FRAMEBUF && (v_mbox[idx + 1] & (1UL << 31)))
            {    
                size_t s_page_idx, e_page_idx, num_pages;
                void *ttbr;

                asm volatile("mrs %0, ttbr0_el1" : "=r"(ttbr));
                s_page_idx = (size_t)USR_FRAMEBUF_START >> 12;
                num_pages = (v_mbox[idx + 3] - 1) / PAGE_SIZE + 1;
                e_page_idx = s_page_idx + num_pages;
                // Map the address of frame buffer for user process
                // The real address of frame buffer is the return value & 0x3fffffff
                fill_page_table(p2v_trans_kernel(ttbr), s_page_idx, e_page_idx, v_mbox[idx + 2] & 0x3fffffff, PD_USER_ACCESS | PD_ACCESS);
                add_section(get_current(), DEVICE, USR_FRAMEBUF_START, num_pages * PAGE_SIZE);
                v_mbox[idx + 2] = (uint32_t)(uintptr_t)USR_FRAMEBUF_START;
            }

            idx += v_mbox[idx] / sizeof(uint32_t) + 2;
        }

        return true;
    }

    return false;
}

static void mailbox_request(int n_buf, uint32_t tag, unsigned char channel, uint32_t *data)
{
    int n = n_buf + 6;
    __attribute__((aligned(16))) uint32_t mailbox[n];
    mailbox[0] = n * 4; // buffer size in bytes
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = tag; // tag identifier
    mailbox[3] = n_buf * sizeof(uint32_t); // maximum of request and response value buffer's length.
    mailbox[4] = TAG_REQUEST_CODE;
    for (int i = 5; i < n - 1; i++)
        mailbox[i] = 0; // buffer
    mailbox[n - 1] = END_TAG;

    mbox_call(channel, mailbox);

    for (int i = 0; i < n_buf; i++)
        data[i] = mailbox[5 + i];
}

static void get_board_revision()
{
    int n_buf = 1;
    uint32_t data[n_buf];
    mailbox_request(n_buf, GET_BOARD_REVISION, 8, data);

    printf("Board revision: 0x%x\r\n", data[0]);
}

static void get_memory_info()
{
    int n_buf = 2;
    uint32_t data[n_buf];
    mailbox_request(n_buf, GET_ARM_MEMORY, 8, data);

    printf("ARM memory base address: 0x%x\r\n", data[0]);
    printf("ARM memory size: 0x%x\r\n", data[1]);
}

void mailbox_info()
{
    printf("Mailbox info:\r\n");
    get_board_revision();
    get_memory_info();
    return;
}