#include "uart.h"
#include "utils.h"

#define MAGIC 0x544f4f42

extern void* _skernel;

int main()
{
    uart_init();
    
    while (true)
    {
        char buf[4];
        uint32_t magic, data_len, checksum;
        uint32_t sum = 0;

        uart_read(buf, 4, RAW_MODE);
        magic = *(uint32_t*)buf;
        uart_read(buf, 4, RAW_MODE);
        data_len = *(uint32_t*)buf;
        uart_read(buf, 4, RAW_MODE);
        checksum = *(uint32_t*)buf;

        char data[data_len];
        uart_read(data, data_len, RAW_MODE);
        
        for (int i = 0; i < data_len; i++)
            sum += (uint32_t)(unsigned char)data[i];

        if (magic != MAGIC)
            uart_write_string("Invalid file\r\n");
        else if (sum != checksum)
            uart_write_string("File corrupted\r\n");
        else
        {
            uart_write_string("File received\r\n");
            memcpy((void*)_skernel, (void*)data, data_len);
            asm volatile ("adr x28, _skernel");
            asm volatile ("ldr x28, [x28]");
            asm volatile ("br x28");
            break;
        }
    }
    return 0;
}