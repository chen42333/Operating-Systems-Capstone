#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include "utils.h"

#define GPFSEL1 (void*)0x3f200004
#define GPPUD (void*)0x3f200094
#define GPPUDCLK0 (void*)0x3f200098

#define AUXENB (void*)0x3f215004
#define AUX_MU_CNTL_REG (void*)0x3f215060
#define AUX_MU_IER_REG (void*)0x3f215044 
#define AUX_MU_LCR_REG (void*)0x3f21504c
#define AUX_MU_MCR_REG (void*)0x3f215050 
#define AUX_MU_BAUD_REG (void*)0x3f215068
#define AUX_MU_IIR_REG (void*)0x3f215048
#define AUX_MU_LSR_REG (void*)0x3f215054
#define AUX_MU_IO_REG (void*)0x3f215040
#define IRQs1 (void*)0x3f00b210

#define STRING_MODE 0
#define RAW_MODE 1

#define IO_SYNC 0
#define IO_ASYNC 1

#define BUFLEN 4096

struct ring_buf
{
    int producer_idx;
    int consumer_idx;
    char buf[BUFLEN];
};

extern struct ring_buf r_buf, w_buf;

void enable_uart_int();
void uart_init();
int uart_write_char(char c);
int uart_write_string(char *str);
int uart_write_hex(uint64_t num, uint32_t size);
int uart_write_dec(uint64_t num);
int uart_read(char *str, uint32_t size, int mode);

inline static void ring_buf_init(struct ring_buf *rb)
{
    rb->producer_idx = 0; // The next position to put
    rb->consumer_idx = 0; // The next position to read
}

inline static bool ring_buf_full(struct ring_buf *rb)
{
    // Wasting 1 byte to differentiate empty/full
    return (rb->producer_idx + 1) % sizeof(rb->buf) == rb->consumer_idx;
}

inline static bool ring_buf_empty(struct ring_buf *rb)
{
    return rb->producer_idx == rb->consumer_idx;
}

inline static void ring_buf_produce(struct ring_buf *rb, char c)
{
    while (ring_buf_full(rb)) ;

    rb->buf[rb->producer_idx++] = c;
    rb->producer_idx %= sizeof(rb->buf);
}

inline static char ring_buf_consume(struct ring_buf *rb)
{
    char ret;

    while (ring_buf_empty(rb)) ;

    ret = rb->buf[rb->consumer_idx++];
    rb->consumer_idx %= sizeof(rb->buf);

    return ret;
}

inline static int uart_write_newline()
{
    return uart_write_string("\r\n");
}

inline static void enable_read_int()
{
    uint32_t data;

    data = get32(AUX_MU_IER_REG);
    data |= 0b1;
    set32(AUX_MU_IER_REG, data);
}

inline static void disable_read_int()
{
    uint32_t data;

    data = get32(AUX_MU_IER_REG);
    data &= ~0b1;
    set32(AUX_MU_IER_REG, data);
}

inline static void enable_write_int()
{
    uint32_t data;

    data = get32(AUX_MU_IER_REG);
    data |= 0b10;
    set32(AUX_MU_IER_REG, data);
}

inline static void disable_write_int()
{
    uint32_t data;

    data = get32(AUX_MU_IER_REG);
    data &= ~0b10;
    set32(AUX_MU_IER_REG, data);
}

inline static void clear_read_fifo()
{
    uint32_t data;

    data = get32(AUX_MU_IIR_REG);
    data |= 0b10;
    set32(AUX_MU_IIR_REG, data);
}

inline static void clear_write_fifo()
{
    uint32_t data;

    data = get32(AUX_MU_IIR_REG);
    data |= 0b100;
    set32(AUX_MU_IIR_REG, data);
}

inline static void err(char *str)
{
    uart_write_string("Error:\t");
    uart_write_string(str);
    return;
}

#endif