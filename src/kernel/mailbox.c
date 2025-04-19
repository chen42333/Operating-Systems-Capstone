#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "printf.h"

int mbox_call(unsigned char channel, uint32_t *mailbox)
{
    uint32_t data = ((unsigned long)mailbox & ~0xf) | channel;

    while (get32(MAILBOX_STATUS) & MAILBOX_FULL) ;
    set32(MAILBOX_WRITE, data);
    while (get32(MAILBOX_STATUS) & MAILBOX_EMPTY) ;
    while (get32(MAILBOX_READ) != data) ;

    return mailbox[1] == REQUEST_SUCCEED;
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