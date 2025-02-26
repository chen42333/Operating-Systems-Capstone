#include <stdint.h>
#include "uart.h"

void uart_init()
{
    volatile uint32_t *ptr = (uint32_t*)GPFSEL1;
    // Set GPIO 14, 15 to ALT5
    *ptr &= ~((7 << 12) | (7 << 15));
    *ptr |= (2 << 12) | (2 << 15);
    
    ptr = (uint32_t*)GPPUD;
    *ptr &= ~3;
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    ptr = (uint32_t*)GPPUDCLK0;
    *ptr = (1 << 14) | (1 << 15);
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    ptr = (uint32_t*)GPPUD;
    *ptr = 0;
    ptr = (uint32_t*)GPPUDCLK0;
    *ptr = 0;

    ptr = (uint32_t*)AUXENB;
    *ptr = 1;
    ptr = (uint32_t*)AUX_MU_CNTL_REG;
    *ptr = 0;
    ptr = (uint32_t*)AUX_MU_IER_REG;
    *ptr = 0;
    ptr = (uint32_t*)AUX_MU_LCR_REG;
    *ptr = 3;
    ptr = (uint32_t*)AUX_MU_MCR_REG;
    *ptr = 0;
    ptr = (uint32_t*)AUX_MU_BAUD_REG;
    *ptr = 270;
    ptr = (uint32_t*)AUX_MU_IIR_REG;
    *ptr = 6;
    ptr = (uint32_t*)AUX_MU_CNTL_REG;
    *ptr = 3;

    return;
}

int uart_write_char(char c)
{
    volatile uint32_t *lsr = (uint32_t*)AUX_MU_LSR_REG;
    char *io = (char*)AUX_MU_IO_REG;

    while (!(*lsr & (1 << 5))) ;
    *io = c;

    return 0;
}

int uart_write_string(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
       uart_write_char(str[i]);

    return 0;
}

int uart_write_hex(unsigned int num)
{
    char buf[sizeof(unsigned int) * 2 + 1];

    uart_write_string("0x");
    for (int i = sizeof(unsigned int) * 2 - 1; i >= 0 ; i--)
    {
        unsigned int byte = num % 16;
        if (byte < 10)
            buf[i] = byte + '0';
        else
            buf[i] = byte - 10 + 'a';
        num >>= 4;
    }
    
    buf[sizeof(unsigned int) * 2] = '\0';
    uart_write_string(buf);

    return 0;
}

int uart_read(char *str, unsigned int size)
{
    volatile uint32_t *lsr = (uint32_t*)AUX_MU_LSR_REG;
    char *io = (char*)AUX_MU_IO_REG;
    int i;
    char c;

    for (i = 0; i < size - 1; i++)
    {
        while (!(*lsr & 1)) ;
        c = *io;
        
        if (c == 0x7f || c == 8) // backspace
        {
            if (i > 0)
            {
                i -= 2;
                uart_write_string("\b \b");
            }
            else
                i--;
        }
        else if (c == '\r')
        {
            uart_write_string("\r\n");
            break;
        }
        else if (c == '\0' || c == '\n')
        {
            uart_write_char(c);
            break;
        } 
        else
        {
            uart_write_char(c);
            str[i] = c;
        }
    }
    str[i] = '\0';

    return i;
}