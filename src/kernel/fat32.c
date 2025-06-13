#include "fat32.h"
#include "sd.h"

static uint8_t fat32_start_blk;

void parse_fat32()
{
    uint8_t buf[BLK_SZ];

    readblock(0, buf);
    if (buf[0x1fe] != 0x55 || buf[0x1ff] != 0xaa)
    {
        err("Invalid MBR\r\n");
        return;
    }
    for (int i = 0x1be; i < 0x1fe; i += 0x10)
    {
        unsigned int *tmp = (unsigned int*)&buf[i];

        if ((tmp[0] & 0xff) == 0x80) // Boot indiactor
        {
            if ((tmp[1] & 0xff) != 0xb) // Partition Type I
            {
                err("Not MBR partitioned\r\n");
                return;
            }
            fat32_start_blk = tmp[2]; // Starting LBA Address (Relative Sectors)

            break;
        }
        
        // printf("Boot indiactor: 0x%x\r\n", tmp[0] & 0xff);
        // printf("Starting CHS Address: 0x%x\r\n", tmp[0] >> 8);
        // printf("Partition Type ID: 0x%x\r\n", tmp[1] & 0xff);
        // printf("Ending CHS Address: 0x%x\r\n", tmp[1] >> 8);
        // printf("Starting LBA Address (Relative Sectors): 0x%x\r\n", tmp[2]);
        // printf("Total Sectors in Partition: 0x%x\r\n", tmp[3]);
        // printf("\r\n");
    }
}