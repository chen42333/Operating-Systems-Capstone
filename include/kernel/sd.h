#ifndef __SD_H
#define __SD_H

#include "utils.h"

#define BLK_SZ 512

// SD card command
#define GO_IDLE_STATE 0
#define SEND_OP_CMD 1
#define ALL_SEND_CID 2
#define SEND_RELATIVE_ADDR 3
#define SELECT_CARD 7
#define SEND_IF_COND 8
#define VOLTAGE_CHECK_PATTERN 0x1aa
#define STOP_TRANSMISSION 12
#define SET_BLOCKLEN 16
#define READ_SINGLE_BLOCK 17
#define WRITE_SINGLE_BLOCK 24
#define SD_APP_OP_COND 41
#define SDCARD_3_3V (1 << 21)
#define SDCARD_ISHCS (1 << 30)
#define SDCARD_READY (1 << 31)
#define APP_CMD 55

// gpio
#define GPIO_BASE (MMIO_BASE + 0x200000)
#define GPIO_GPFSEL4 (GPIO_BASE + 0x10)
#define GPIO_GPFSEL5 (GPIO_BASE + 0x14)
#define GPIO_GPPUD (GPIO_BASE + 0x94)
#define GPIO_GPPUDCLK1 (GPIO_BASE + 0x9c)

// sdhost
#define SDHOST_BASE (MMIO_BASE + 0x202000)
#define SDHOST_CMD (SDHOST_BASE + 0)
#define SDHOST_READ 0x40
#define SDHOST_WRITE 0x80
#define SDHOST_LONG_RESPONSE 0x200
#define SDHOST_NO_REPONSE 0x400
#define SDHOST_BUSY 0x800
#define SDHOST_NEW_CMD 0x8000
#define SDHOST_ARG (SDHOST_BASE + 0x4) 
#define SDHOST_TOUT (SDHOST_BASE + 0x8)
#define SDHOST_TOUT_DEFAULT 0xf00000
#define SDHOST_CDIV (SDHOST_BASE + 0xc)
#define SDHOST_CDIV_MAXDIV 0x7ff
#define SDHOST_CDIV_DEFAULT 0x148
#define SDHOST_RESP0 (SDHOST_BASE + 0x10)
#define SDHOST_RESP1 (SDHOST_BASE + 0x14)
#define SDHOST_RESP2 (SDHOST_BASE + 0x18)
#define SDHOST_RESP3 (SDHOST_BASE + 0x1c)
#define SDHOST_HSTS (SDHOST_BASE + 0x20)
#define SDHOST_HSTS_MASK (0x7f8)
#define SDHOST_HSTS_ERR_MASK (0xf8)
#define SDHOST_HSTS_DATA (1 << 0)
#define SDHOST_PWR (SDHOST_BASE + 0x30)
#define SDHOST_DBG (SDHOST_BASE + 0x34)
#define SDHOST_DBG_FSM_DATA 1
#define SDHOST_DBG_FSM_MASK 0xf
#define SDHOST_DBG_MASK (0x1f << 14 | 0x1f << 9)
#define SDHOST_DBG_FIFO (0x4 << 14 | 0x4 << 9)
#define SDHOST_CFG (SDHOST_BASE + 0x38)
#define SDHOST_CFG_DATA_EN (1 << 4)
#define SDHOST_CFG_SLOW (1 << 3)
#define SDHOST_CFG_INTBUS (1 << 1)
#define SDHOST_SIZE (SDHOST_BASE + 0x3c)
#define SDHOST_DATA (SDHOST_BASE + 0x40)
#define SDHOST_CNT (SDHOST_BASE + 0x50)

// helper
#define set(io_addr, val) \
  asm volatile("str %w1, [%0]" ::"r"(io_addr), "r"(val) : "memory");

#define get(io_addr, val) \
  asm volatile("ldr %w0, [%1]" : "=r"(val) : "r"(io_addr) : "memory");

static inline void delay(unsigned long tick) {
  while (tick--) {
    asm volatile("nop");
  }
}

void readblock(int block_idx, void* buf);
void writeblock(int block_idx, void* buf);
void sd_init();

#endif