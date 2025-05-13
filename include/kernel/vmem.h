#ifndef __VMEM_H
#define __VMEM_H

#include "utils.h"
#include "paging.h"

#define MMIO_START 0x3f000000

void finer_granu_paging();
void *v2p_trans(void *virtual_addr);

#endif