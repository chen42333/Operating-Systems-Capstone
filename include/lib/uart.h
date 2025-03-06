#ifndef __UART_H
#define __UART_H

#include <stdint.h>

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

#define STRING_MODE 0
#define RAW_MODE 1

void uart_init();
int uart_write_char(char c);
int uart_write_string(char *str);
int uart_write_hex(uint64_t num, uint32_t size);
int uart_write_dec(uint64_t num);
int uart_read(char *str, uint32_t size, int mode);

inline static int uart_write_newline()
{
    return uart_write_string("\r\n");
}

#endif