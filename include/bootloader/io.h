#ifndef __IO_H
#define __IO_H

#include <stdint.h>

#define true 1
#define false 0

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

void uart_init();
int uart_write_string(char *str);
int uart_read_raw(char *str, uint32_t size);

inline static void set32(void *addr, uint32_t value) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    *ptr = value;
}

inline static uint32_t get32(void *addr) {
    volatile uint32_t *ptr = (uint32_t*)addr;
    return *ptr;
}

inline static void set8(void *addr, char value) {
    volatile char *ptr = (char*)addr;
    *ptr = value;
}

inline static char get8(void *addr) {
    volatile char *ptr = (char*)addr;
    return *ptr;
}

#endif