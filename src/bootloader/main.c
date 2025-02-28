#include "uart.h"
#include "utils.h"

#define MAGIC 0x544f4f42

int main()
{
    char buf[5];
    uint32_t magic, data_len, checksum;
    uint32_t sum = 0;

    uart_read(buf, 5, RAW_MODE);
    magic = str2u32(buf);
    uart_read(buf, 5, RAW_MODE);
    data_len = str2u32(buf);
    uart_read(buf, 5, RAW_MODE);
    checksum = str2u32(buf);

    char data[data_len + 1];
    uart_read(data, data_len + 1, RAW_MODE);
    
    for (int i = 0; i < data_len; i++)
        sum += (uint32_t)(unsigned char)data[i];

    if (magic != MAGIC)
        uart_write_string("Invalid file\r\n");
    else if (sum != checksum)
        uart_write_string("File corrupted\r\n");
    else
        uart_write_string("File received\r\n");

    while (true) ;
    return 0;
}