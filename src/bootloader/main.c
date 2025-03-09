#include "uart.h"
#include "utils.h"

#define MAGIC 0x544f4f42

extern char _skernel[];

int main(void *_dtb_addr)
{
    uart_init();
    
    while (true)
    {
        char buf[4];
        uint32_t magic, data_len, checksum;
        uint32_t sum = 0;

        uart_read(buf, 4, RAW_MODE, IO_SYNC);
        magic = *(uint32_t*)buf;
        uart_read(buf, 4, RAW_MODE, IO_SYNC);
        data_len = *(uint32_t*)buf;
        uart_read(buf, 4, RAW_MODE, IO_SYNC);
        checksum = *(uint32_t*)buf;

        char data[data_len];
        uart_read(data, data_len, RAW_MODE, IO_SYNC);
        
        for (int i = 0; i < data_len; i++)
            sum += (uint32_t)(unsigned char)data[i];

        if (magic != MAGIC)
            uart_write_string("Invalid file\r\n", IO_SYNC);
        else if (sum != checksum)
            uart_write_string("File corrupted\r\n", IO_SYNC);
        else
        {
            uart_write_string("File received\r\n", IO_SYNC);
            memcpy((void*)_skernel, (void*)data, data_len);
            asm volatile ("mov x0, %0" :: "r"(_dtb_addr));
            asm volatile ("b _skernel");
            break;
        }
    }
    return 0;
}