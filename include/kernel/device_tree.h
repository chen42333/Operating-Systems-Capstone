#ifndef __DEVICE_TREE_H
#define __DEVICE_TREE_H

#include <stdint.h>
#include "utils.h"

#define FDT_BEGIN_NODE 0x00000001
#define FDT_END_NODE 0x00000002
#define FDT_PROP 0x00000003
#define FDT_NOP 0x00000004
#define FDT_END 0x00000009

struct fdt_header {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
};

struct fdt_node_comp {
    uint32_t token;
    char data[];
};

struct fdt_node_prop {
    uint32_t len;
    uint32_t nameoff;
};

inline static void dtb_get_len() {
    dtb_len = big2host(((struct fdt_header*)dtb_addr)->totalsize);
}

void fdt_traverse(bool (*func)(void*, char*));

#endif