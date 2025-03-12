#include "utils.h"
#include "uart.h"

#ifndef UART_SYNC
struct ring_buf r_buf, w_buf;

void enable_uart_int()
{
    uint32_t data;

    data = 0b1100;
    set32(AUX_MU_IER_REG, data);

    data = get32(IRQs1);
    data |= (1 << 29);
    set32(IRQs1, data);

    // Initialize read/write buffer
    ring_buf_init(&r_buf, CHAR);
    ring_buf_init(&w_buf, CHAR);

    // Enable interrupt in EL1
    asm volatile("msr daifclr, 0xf");
}
#endif

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

    set32(AUXENB, 1); // enable mini UART
    set32(AUX_MU_CNTL_REG, 0); // disable transmitter and receiver during configuration
    set32(AUX_MU_IER_REG, 0); // disable interrupt because currently it does’t need interrupt
    set32(AUX_MU_LCR_REG, 3); // set the data size to 8 bit
    set32(AUX_MU_MCR_REG, 0); // don’t need auto flow control
    set32(AUX_MU_BAUD_REG, 270); // set baud rate to 115200
    set32(AUX_MU_IIR_REG, 6);
    set32(AUX_MU_CNTL_REG, 3); // enable the transmitter and receiver
}

int uart_write_char(char c)
{
#ifdef UART_SYNC
    while (!(get32(AUX_MU_LSR_REG) & (1 << 5))) ;
    set8(AUX_MU_IO_REG, c);
#else
    ring_buf_produce(&w_buf, &c, CHAR);
    enable_write_int();
#endif

    return 0;
}

int uart_write_string(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
       uart_write_char(str[i]);

    return 0;
}

int uart_write_hex(uint64_t num, uint32_t size)
{
    char buf[size * 2 + 1];

    uart_write_string("0x");
    for (int i = size * 2 - 1; i >= 0 ; i--)
    {
        uint64_t byte = num % 16;
        if (byte < 10)
            buf[i] = byte + '0';
        else
            buf[i] = byte - 10 + 'a';
        num >>= 4;
    }
    
    buf[size * 2] = '\0';
    uart_write_string(buf);

    return 0;
}

int uart_write_dec(uint64_t num)
{
    char buf[21];
    int i;

    for (i = 0; num > 0; i++)
    {
        buf[i] = num % 10 + '0';
        num /= 10;
    }

    for (i--; i >= 0; i--)
        uart_write_char(buf[i]);

    return 0;
}

int uart_read(char *str, uint32_t size, int mode)
{
    int i;
    char c;

#ifndef UART_SYNC
    enable_read_int();
#endif

    for (i = 0; i < size - 1; i++)
    {
#ifdef UART_SYNC
        while (!(get32(AUX_MU_LSR_REG) & 1)) ;
        c = get8(AUX_MU_IO_REG);
#else
        ring_buf_consume(&r_buf, &c, CHAR);
#endif
        
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
                uart_write_newline();
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
#ifdef UART_SYNC
    while (!(get32(AUX_MU_LSR_REG) & 1)) ;
    str[i] = get8(AUX_MU_IO_REG);
#else
    ring_buf_consume(&r_buf, &str[i], CHAR);
#endif
    }
    else if (mode == STRING_MODE)
        str[i] = '\0';

#ifndef UART_SYNC
    disable_read_int();
#endif

    return i;
}