#ifndef __UART_H
#define __UART_H

#include <stdint.h>
#include "utils.h"
#include "ring_buf.h"

#define GPFSEL1 (void*)(0x3f200004 + v_kernel_space)
#define GPPUD (void*)(0x3f200094 + v_kernel_space)
#define GPPUDCLK0 (void*)(0x3f200098 + v_kernel_space)

#define AUXENB (void*)(0x3f215004 + v_kernel_space)
#define AUX_MU_CNTL_REG (void*)(0x3f215060 + v_kernel_space)
#define AUX_MU_IER_REG (void*)(0x3f215044 + v_kernel_space)
#define AUX_MU_LCR_REG (void*)(0x3f21504c + v_kernel_space)
#define AUX_MU_MCR_REG (void*)(0x3f215050  + v_kernel_space)
#define AUX_MU_BAUD_REG (void*)(0x3f215068 + v_kernel_space)
#define AUX_MU_IIR_REG (void*)(0x3f215048 + v_kernel_space)
#define AUX_MU_LSR_REG (void*)(0x3f215054 + v_kernel_space)
#define AUX_MU_IO_REG (void*)(0x3f215040 + v_kernel_space)
#define IRQs1 (void*)(0x3f00b210 + v_kernel_space)


void enable_uart_int();
void uart_init();
int uart_write_char(char c);
size_t uart_write(const char buf[], size_t size);
size_t uart_read_string(char *str, size_t size);
size_t uart_read(char buf[], size_t size);

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