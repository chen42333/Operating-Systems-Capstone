#include "io.h"

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

int uart_read_raw(char *str, uint32_t size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        while (!(get32(AUX_MU_LSR_REG) & 1)) ;
        str[i] = get8(AUX_MU_IO_REG);
    }

    return i;
}