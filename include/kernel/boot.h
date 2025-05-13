#ifndef __BOOT_H
#define __BOOT_H

#include <stdint.h>

#define PM_PASSWORD (uint32_t)0x5a000000
#define PM_RSTC (void*)(0x3f10001c + v_kernel_space)
#define PM_WDOG (void*)(0x3f100024 + v_kernel_space)

void reset(int tick);
void cancel_reset();

#endif