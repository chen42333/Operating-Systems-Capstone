#ifndef __PAGING_H
#define __PAGING_H

// The size of each table is 4KB
#define PGD_ADDR 0x100000
#define PUD_ADDR 0x101000

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE 0b00000000
#define MAIR_NORMAL_NOCACHE 0b01000100
#define MAIR_IDX_DEVICE_nGnRnE 0
#define MAIR_IDX_NORMAL_NOCACHE 1


#define PD_NX_EL0 (1ULL << 54)
#define PD_NX_EL1 (1ULL << 53)
#define PD_ACCESS (1ULL << 10)
#define PD_RDONLY (1ULL << 7)
#define PD_USER_ACCESS (1ULL << 6)
#define PD_TABLE 0b11
#define PD_BLOCK 0b01
#define PD_PAGE 0b11
#define BOOT_PGD_ATTR PD_TABLE
#define BOOT_PUD_ATTR (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)

#endif