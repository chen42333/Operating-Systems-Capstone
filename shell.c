#define true 1
#define false 0
#define STRLEN 256

#define _GPFSEL1 (unsigned int *)0x3f200004
#define _GPPUD (unsigned int *)0x3f200094
#define _GPPUDCLK0 (unsigned int *)0x3f200098
#define _AUXENB (unsigned int *)0x3f215004
#define _AUX_MU_CNTL_REG (unsigned int *)0x3f215060
#define _AUX_MU_IER_REG (unsigned int *)0x3f215044 
#define _AUX_MU_LCR_REG (unsigned int *)0x3f21504c
#define _AUX_MU_MCR_REG (unsigned int *)0x3f215050 
#define _AUX_MU_BAUD_REG (unsigned int *)0x3f215068
#define _AUX_MU_IIR_REG (unsigned int *)0x3f215048
#define _AUX_MU_LSR_REG (unsigned int *)0x3f215054
#define _AUX_MU_IO_REG (char *)0x3f215040

int strcmp(const char *str1, const char *str2)
{
    for (int i = 0; ; i++)
    {

        if (str1[i] < str2[i])
            return -1;
        if (str1[i] > str2[i])
            return 1;
        if (str1[i] == '\0' && str2[i] == '\0')
            break;
    }
    return 0;
}

void uart_init()
{
    unsigned int *val = _GPFSEL1;
    // Set GPIO 14, 15 to ALT5
    *val &= ~((7 << 12) | (7 << 15));
    *val |= (2 << 12) | (2 << 15);
    
    val = _GPPUD;
    *val &= ~3;
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    val = _GPPUDCLK0;
    *val = (1 << 14) | (1 << 15);
    for (int i = 0; i < 150; i++)
        asm volatile("nop");
    val = _GPPUD;
    *val = 0;
    val = _GPPUDCLK0;
    *val = 0;

    val = _AUXENB;
    *val = 1;
    val = _AUX_MU_CNTL_REG;
    *val = 0;
    val = _AUX_MU_IER_REG;
    *val = 0;
    val = _AUX_MU_LCR_REG;
    *val = 3;
    val = _AUX_MU_MCR_REG;
    *val = 0;
    val = _AUX_MU_BAUD_REG;
    *val = 270;
    val = _AUX_MU_IIR_REG;
    *val = 6;
    val = _AUX_MU_CNTL_REG;
    *val = 3;

    return;
}

int uart_write_char(char c)
{
    unsigned int *lsr = _AUX_MU_LSR_REG;
    char *io = _AUX_MU_IO_REG;

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

int uart_read(char *str, unsigned int size)
{
    unsigned int *lsr = _AUX_MU_LSR_REG;
    char *io = _AUX_MU_IO_REG;
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
            uart_write_char('\n');
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

int main()
{
    char cmd[STRLEN];

    uart_init();
    while (true)
    {
        uart_write_string("# ");
        uart_read(cmd, STRLEN);
        if (!strcmp("help", cmd))
            uart_write_string("help\t: print this help menu\n"
                    "hello\t: print Hello World!\n"
                    "mailbox\t: print hardware's information\n");
        else if (!strcmp("hello", cmd))
            uart_write_string("Hello World!\n");
        else if (!strcmp("mailbox", cmd))
            ;
        else
            uart_write_string("Invalid command\n");
    }
    return 0;
}