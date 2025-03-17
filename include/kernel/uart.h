#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include "utils.h"
#include "ring_buf.h"

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


void enable_uart_int();
void uart_init();
int uart_write_char(char c);
int uart_read_string(char *str, uint32_t size);

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

#endif