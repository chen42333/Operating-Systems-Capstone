#include "uart.h"

void exception_entry()
{
    uint64_t value;

    asm volatile ("mrs %0, spsr_el1" : "=r"(value));
    uart_write_string("SPSR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
    asm volatile ("mrs %0, elr_el1" : "=r"(value));
    uart_write_string("ELR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
    asm volatile ("mrs %0, esr_el1" : "=r"(value));
    uart_write_string("ESR_EL1: ");
    uart_write_hex(value, sizeof(uint64_t));
    uart_write_string("\r\n");
}

void elasped_time()
{
    uint64_t count, freq;

    asm volatile ("mrs %0, cntpct_el0" : "=r"(count));
    asm volatile ("mrs %0, cntfrq_el0" : "=r"(freq));
    uart_write_dec(count / freq);
    uart_write_string(" seconds after booting\r\n");
}

void tx_int()
{
    char c;

    c = ring_buf_consume(&w_buf);
    set8(AUX_MU_IO_REG, c);
    
    if (ring_buf_empty(&w_buf))
        disable_write_int();
}

void rx_int()
{
    char c;

    c = get8(AUX_MU_IO_REG);
    ring_buf_produce(&r_buf, c);
}