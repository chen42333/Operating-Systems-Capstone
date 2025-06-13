#ifndef __MAILBOX_H
#define __MAILBOX_H

#include "utils.h"

#define MAILBOX_BASE (void*)(MMIO_BASE + 0xb880)

#define MAILBOX_READ MAILBOX_BASE
#define MAILBOX_STATUS (MAILBOX_BASE + 0x18)
#define MAILBOX_WRITE (MAILBOX_BASE + 0x20)

#define MAILBOX_EMPTY 0x40000000
#define MAILBOX_FULL 0x80000000

#define GET_BOARD_REVISION 0x00010002
#define GET_ARM_MEMORY 0x00010005
#define ALLOC_FRAMEBUF 0x00040001
#define REQUEST_CODE 0x00000000
#define REQUEST_SUCCEED 0x80000000
#define REQUEST_FAILED 0x80000001
#define TAG_REQUEST_CODE 0x00000000
#define END_TAG 0x00000000

#define MBOX_CH_PROP 8

struct framebuffer_info 
{
    unsigned int width;
    unsigned int height;
    unsigned int pitch;
    unsigned int isrgb;
};

extern void *fb_addr;
extern struct framebuffer_info fb_info;

int mbox_call(unsigned char channel, uint32_t *mailbox); // Virtual
void mailbox_info();
void get_fb(size_t *size);

#endif