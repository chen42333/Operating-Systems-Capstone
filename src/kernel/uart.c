#include "utils.h"
#include "uart.h"
#include "printf.h"
#include "process.h"
#include "interrupt.h"

struct ring_buf r_buf, w_buf;
char read_buf[BUFLEN], write_buf[BUFLEN];

void enable_uart_int()
{
    uint32_t data;

    data = 0b1100;
    set32(AUX_MU_IER_REG, data);

    data = get32(IRQs1);
    data |= (1 << 29);
    set32(IRQs1, data);

    // Initialize read/write buffer
    ring_buf_init(&r_buf, read_buf);
    ring_buf_init(&w_buf, write_buf);

    // Enable interrupt in EL1
    enable_int();
}

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
    ring_buf_produce(&w_buf, &c, CHAR);
    enable_write_int();

    return 0;
}

size_t uart_write(const char buf[], size_t size)
{
    size_t i;

    if (ring_buf_remain_e(&w_buf) < size)
        wait(WRITE, size);

    for (i = 0; i < size; i++)
        uart_write_char(buf[i]);

    return i;
}

size_t uart_read_string(char *str, size_t size)
{
    size_t i;
    char c;

    enable_read_int();

    for (i = 0; i < size - 1; i++)
    {
        ring_buf_consume(&r_buf, &c, CHAR);

        if (c == 0x7f || c == 8) // backspace
        {
            if (i > 0)
            {
                i -= 2;
                printf("\b \b");
            }
            else
                i--;
        }
        else if (c == '\r')
        {
            printf("\r\n");
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

    disable_read_int();

    return i;
}

size_t uart_read(char buf[], size_t size)
{
    size_t i;
    char c;

    enable_read_int();

    if (ring_buf_num_e(&r_buf) < size)
        wait(READ, size);

    for (i = 0; i < size; i++)
    {
        ring_buf_consume(&r_buf, &c, CHAR);
        buf[i] = c;
    }

    disable_read_int();

    return i;
}