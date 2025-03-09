#include "utils.h"
#include "uart.h"
#include "mailbox.h"

static void mailbox_call(uint32_t *mailbox)
{
    uint32_t data = ((unsigned long)mailbox & ~0xf) | 8;

    while (get32(MAILBOX_STATUS) & MAILBOX_FULL) ;
    set32(MAILBOX_WRITE, data);
    while (get32(MAILBOX_STATUS) & MAILBOX_EMPTY) ;
    while (get32(MAILBOX_READ) != data) ;
}

static void mailbox_request(int n_buf, uint32_t tag, uint32_t *data)
{
    int n = n_buf + 6;
    __attribute__((aligned(16))) uint32_t mailbox[n];
    mailbox[0] = n * 4;
    mailbox[1] = REQUEST_CODE;
    mailbox[2] = tag;
    mailbox[3] = n_buf * sizeof(uint32_t);
    mailbox[4] = TAG_REQUEST_CODE;
    for (int i = 5; i < n - 1; i++)
        mailbox[i] = 0; // buffer
    mailbox[n - 1] = END_TAG;

    mailbox_call(mailbox);

    for (int i = 0; i < n_buf; i++)
        data[i] = mailbox[5 + i];
}

static void get_board_revision()
{
    int n_buf = 1;
    uint32_t data[n_buf];
    mailbox_request(n_buf, GET_BOARD_REVISION, data);

    uart_write_string("Board revision: ", IO_ASYNC);
    uart_write_hex(data[0], sizeof(uint32_t), IO_ASYNC);
    uart_write_string("\r\n", IO_ASYNC);
}

static void get_memory_info()
{
    int n_buf = 2;
    uint32_t data[n_buf];
    mailbox_request(2, GET_ARM_MEMORY, data);

    uart_write_string("ARM memory base address: ", IO_ASYNC);
    uart_write_hex(data[0], sizeof(uint32_t), IO_ASYNC);
    uart_write_string("\r\n", IO_ASYNC);
    uart_write_string("ARM memory size: ", IO_ASYNC);
    uart_write_hex(data[1], sizeof(uint32_t), IO_ASYNC);
    uart_write_string("\r\n", IO_ASYNC);
}

void mailbox_info()
{
    uart_write_string("Mailbox info:\r\n", IO_ASYNC);
    get_board_revision();
    get_memory_info();
    return;
}