#include "paging.h"

.section .text

.global vmem_init
vmem_init:
    mov x8, lr
    bl tcr_set
    bl mair_set
    bl idt_paging_set
    mov lr, x8
    ret

tcr_set:
    ldr x0, =TCR_CONFIG_DEFAULT
    msr tcr_el1, x0
    ret

mair_set:
    ldr x0, =((MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE << 3)) | \
            (MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE << 3)))
    msr mair_el1, x0
    ret

idt_paging_set:
    ldr x0, =PGD_ADDR // PGD's page frame at 0x0
    ldr x1, =PUD_ADDR // PUD's page frame at 0x1000

    ldr x2, =BOOT_PGD_ATTR
    orr x2, x1, x2 // combine the physical address of next level page with attribute.
    str x2, [x0]

    ldr x2, =BOOT_PUD_ATTR
    mov x3, 0x00000000
    orr x3, x2, x3
    str x3, [x1] // 1st 1GB mapped by the 1st entry of PUD
    mov x3, 0x40000000
    orr x3, x2, x3
    str x3, [x1, 8] // 2nd 1GB mapped by the 2nd entry of PUD

    msr ttbr0_el1, x0 // load PGD to the bottom translation-based register.
    msr ttbr1_el1, x0 // also load PGD to the upper translation based register.
    mrs x2, sctlr_el1
    orr x2 , x2, 1
    msr sctlr_el1, x2 // enable MMU, cache remains disabled

    ldr x2, =boot_rest // indirect branch to the virtual address
    br x2