#include "uart.h"
#include "mailbox.h"

#define true 1
#define false 0
#define STRLEN 256

void err(char *str)
{
    uart_write_string("Error:\t");
    uart_write_string(str);
    return;
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
            mailbox_info();
        else
            uart_write_string("Invalid command\n");
    }
    return 0;
}