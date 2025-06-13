#include "io.h"

#define MAGIC 0x544f4f42

extern char _skernel[];

int main(void *_dtb_addr) {
    uart_init();
    
    while (true) {
        char buf[4];
        uint32_t magic, data_len, checksum;
        uint32_t sum = 0;

        uart_write_string("Receiving kernel image...\r\n");

        uart_read_raw(buf, 4);
        magic = *(uint32_t*)buf;
        uart_read_raw(buf, 4);
        data_len = *(uint32_t*)buf;
        uart_read_raw(buf, 4);
        checksum = *(uint32_t*)buf;

        uart_read_raw(_skernel, data_len);
        
        for (int i = 0; i < data_len; i++)
            sum += (uint32_t)(unsigned char)_skernel[i];

        if (magic != MAGIC)
            uart_write_string("Invalid file\r\n");
        else if (sum != checksum)
            uart_write_string("File corrupted\r\n");
        else {
            uart_write_string("File received\r\n");
            asm volatile ("mov x0, %0" :: "r"(_dtb_addr));
            asm volatile ("b _skernel");
            break;
        }
    }
    return 0;
}