#include <stdint.h>
#include <stdbool.h>

#define true 1
#define false 0

#define STRLEN 256

#define AUX_MU_LSR_REG (void*)0x3f215054
#define AUX_MU_IO_REG (void*)0x3f215040

inline static uint32_t get32(void *addr)
{
    volatile uint32_t *ptr = (uint32_t*)addr;
    return *ptr;
}

inline static void set8(void *addr, char value) {
    volatile char *ptr = (char*)addr;
    *ptr = value;
}

inline static char get8(void *addr)
{
    volatile char *ptr = (char*)addr;
    return *ptr;
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

int uart_read(char *str, uint32_t size)
{
    int i;
    char c;

    for (i = 0; i < size - 1; i++)
    {
        while (!(get32(AUX_MU_LSR_REG) & 1)) ;
        c = get8(AUX_MU_IO_REG);
        
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

int main()
{
    char cmd[STRLEN];
    char *quit_cmd[] = {"quit", "q", "exit"};
    int size = sizeof(quit_cmd) / sizeof(quit_cmd[0]);
    bool quit = false;

    while (!quit)
    {
        uart_read(cmd, STRLEN);

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