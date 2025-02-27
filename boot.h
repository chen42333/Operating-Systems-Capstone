#ifndef __BOOT_H
#define __BOOT_H

#include <stdint.h>

#define PM_PASSWORD (uint32_t)0x5a000000
#define PM_RSTC (void*)0x3f10001c
#define PM_WDOG (void*)0x3f100024

void reset(int tick);
void cancel_reset();

#endif