#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "printf.h"
#include "vmem.h"

void *fb_addr; // Physical address
struct framebuffer_info fb_info = { 1024, 768, 4096, 1 };

int mbox_call(unsigned char channel, uint32_t *mailbox)
{
    uint32_t *p_mbox = v2p_trans((void*)mailbox, NULL);
    uint32_t *v_mbox = p2v_trans_kernel(p_mbox);
    uint32_t data = ((unsigned long)p_mbox & ~0xf) | channel;

    while (get32(MAILBOX_STATUS) & MAILBOX_FULL) ;
    set32(MAILBOX_WRITE, data);
    while (get32(MAILBOX_STATUS) & MAILBOX_EMPTY) ;
    while (get32(MAILBOX_READ) != data) ;

    // Check whether the caller is a user process
    // Check whether it contains frame buffer allocation request, to translate the buffer addr for user process
    if (get_current() && v_mbox[1] == REQUEST_SUCCEED)
    {
        int idx = 2;
        
        while (v_mbox[idx] != END_TAG)
        {
            if (v_mbox[idx++] == ALLOC_FRAMEBUF && (v_mbox[idx + 1] & (1UL << 31)))
            {    
                size_t s_page_idx, e_page_idx, num_pages;
                void *ttbr;

                fb_addr = (void*)(uintptr_t)(v_mbox[idx + 2] & 0x3fffffff);

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

    if (!get_current())
        return mailbox[1] == REQUEST_SUCCEED;

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
    mailbox_request(n_buf, GET_BOARD_REVISION, MBOX_CH_PROP, data);

    printf("Board revision: 0x%x\r\n", data[0]);
}

static void get_memory_info()
{
    int n_buf = 2;
    uint32_t data[n_buf];
    mailbox_request(n_buf, GET_ARM_MEMORY, MBOX_CH_PROP, data);

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

void get_fb(size_t *size)
{
    unsigned int __attribute__((aligned(16))) mbox[36];

    mbox[0] = 35 * 4;
    mbox[1] = REQUEST_CODE;

    mbox[2] = 0x48003; // set phy wh
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = 1024; // FrameBufferInfo.width
    mbox[6] = 768;  // FrameBufferInfo.height

    mbox[7] = 0x48004; // set virt wh
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = 1024; // FrameBufferInfo.virtual_width
    mbox[11] = 768;  // FrameBufferInfo.virtual_height

    mbox[12] = 0x48009; // set virt offset
    mbox[13] = 8;
    mbox[14] = 8;
    mbox[15] = 0; // FrameBufferInfo.x_offset
    mbox[16] = 0; // FrameBufferInfo.y.offset

    mbox[17] = 0x48005; // set depth
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = 32; // FrameBufferInfo.depth

    mbox[21] = 0x48006; // set pixel order
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1; // RGB, not BGR preferably

    mbox[25] = 0x40001; // get framebuffer, gets alignment on request
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096; // FrameBufferInfo.pointer
    mbox[29] = 0;    // FrameBufferInfo.size

    mbox[30] = 0x40008; // get pitch
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0; // FrameBufferInfo.pitch

    mbox[34] = REQUEST_CODE;

    // this might not return exactly what we asked for, could be
    // the closest supported resolution instead
    if (mbox_call(MBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) 
    {
        mbox[28] &= 0x3FFFFFFF;         // convert GPU address to ARM address
        fb_info.width = mbox[5];        // get actual physical width
        fb_info.height = mbox[6];       // get actual physical height
        fb_info.pitch = mbox[33];       // get number of bytes per line
        fb_info.isrgb = mbox[24];       // get the actual channel order
    } else {
        printf("Unable to set screen resolution to 1024x768x32\r\n");
    }

    fb_addr = (void*)(uintptr_t)mbox[28];
    *size = mbox[29];
}