#include "utils.h"
#include "uart.h"

void uart_init()
{
    uint32_t data;
    // Set GPIO 14, 15 to ALT5
    data = get32(GPFSEL1);
    data &= ~((7 << 12) | (7 << 15));
    data |= (2 << 12) | (2 << 15);
    set32(GPFSEL1, data);
    
    data = get32(GPPUD);
    data &= ~3;
    set32(GPPUD, data);
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    set32(GPPUDCLK0, (1 << 14) | (1 << 15));
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    set32(GPPUD, 0);
    set32(GPPUDCLK0, 0);

    set32(AUXENB, 1);
    set32(AUX_MU_CNTL_REG, 0);
    set32(AUX_MU_IER_REG, 0);
    set32(AUX_MU_LCR_REG, 3);
    set32(AUX_MU_MCR_REG, 0);
    set32(AUX_MU_BAUD_REG, 270);
    set32(AUX_MU_IIR_REG, 6);
    set32(AUX_MU_CNTL_REG, 3);
}

int uart_write_char(char c)
{
    while (!(get32(AUX_MU_LSR_REG) & (1 << 5))) ;
    set8(AUX_MU_IO_REG, c);

    return 0;
}

int uart_write_string(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
       uart_write_char(str[i]);

    return 0;
}

int uart_write_hex(uint32_t num)
{
    char buf[sizeof(uint32_t) * 2 + 1];

    uart_write_string("0x");
    for (int i = sizeof(uint32_t) * 2 - 1; i >= 0 ; i--)
    {
        uint32_t byte = num % 16;
        if (byte < 10)
            buf[i] = byte + '0';
        else
            buf[i] = byte - 10 + 'a';
        num >>= 4;
    }
    
    buf[sizeof(uint32_t) * 2] = '\0';
    uart_write_string(buf);

    return 0;
}

int uart_read(char *str, uint32_t size, int mode)
{
    int i;
    char c;

    for (i = 0; i < size - 1; i++)
    {
        while (!(get32(AUX_MU_LSR_REG) & 1)) ;
        c = get8(AUX_MU_IO_REG);
        
        if (mode == RAW_MODE)
            str[i] = c;
        else if (mode == STRING_MODE)
        {
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
    }

    if (mode == RAW_MODE)
    {
        while (!(get32(AUX_MU_LSR_REG) & 1)) ;
        str[i] = get8(AUX_MU_IO_REG);
    }
    else if (mode == STRING_MODE)
        str[i] = '\0';

    return i;
}