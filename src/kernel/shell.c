#include "utils.h"
#include "uart.h"
#include "mailbox.h"
#include "boot.h"

int main()
{
    char cmd[STRLEN];

    uart_init();
    while (true)
    {
        uart_write_string("# ");
        uart_read(cmd, STRLEN);
        if (!strcmp("help", cmd))
            uart_write_string("help\t: print this help menu\r\n"
                    "hello\t: print Hello World!\r\n"
                    "mailbox\t: print hardware's information\r\n"
                    "reboot\t: reboot the device\r\n");
        else if (!strcmp("hello", cmd))
            uart_write_string("Hello World!\r\n");
        else if (!strcmp("mailbox", cmd))
            mailbox_info();
        else if (!strcmp("reboot", cmd))
                reset(1);
        else
            uart_write_string("Invalid command\r\n");
    }
    return 0;
}