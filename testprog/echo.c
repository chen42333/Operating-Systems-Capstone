#include "utils.h"
#include "uart.h"

int main()
{
    char cmd[256];
    char *quit_cmd[] = {"quit", "q", "exit"};
    int size = sizeof(quit_cmd) / sizeof(quit_cmd[0]);
    bool quit = false;

    while (!quit)
    {

        uart_read(cmd, 256, STRING_MODE);

        for (int i = 0; i < size; i++)
        {
            if (!strcmp(quit_cmd[i], cmd))
            {
                quit = true;
                break;
            }
        }

        if (!quit)
        {
            uart_write_string(cmd);
            uart_write_string("\r\n");
        }
    }
    return 0;
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