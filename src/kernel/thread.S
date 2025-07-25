.section .text
.global switch_to
switch_to:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]
    stp fp, lr, [x0, 16 * 5]
    mov x9, sp
    str x9, [x0, 16 * 6]

    ldp x19, x20, [x1, 16 * 0]
    ldp x21, x22, [x1, 16 * 1]
    ldp x23, x24, [x1, 16 * 2]
    ldp x25, x26, [x1, 16 * 3]
    ldp x27, x28, [x1, 16 * 4]
    ldp fp, lr, [x1, 16 * 5]
    ldr x9, [x1, 16 * 6]
    mov sp, x9

    mov x0, x4

    dsb ish // ensure write has completed
    msr ttbr0_el1, x5 // switch translation based address.
    tlbi vmalle1is // invalidate all TLB entries
    dsb ish // ensure completion of TLB invalidatation
    isb // clear pipeline

    msr elr_el1, x2
    msr spsr_el1, x3
    eret

.global save_regs
save_regs:
    stp x19, x20, [x0, 16 * 0]
    stp x21, x22, [x0, 16 * 1]
    stp x23, x24, [x0, 16 * 2]
    stp x25, x26, [x0, 16 * 3]
    stp x27, x28, [x0, 16 * 4]

    cbnz x1, 1f
    mov x1, fp
1:
    cbnz x2, 2f
    mov x2, lr
2:
    cbnz x3, 3f
    mov x3, sp
3:
    stp x1, x2, [x0, 16 * 5]
    str x3, [x0, 16 * 6]

    ret

.global load_regs
load_regs:
    ldp x19, x20, [x0, 16 * 0]
    ldp x21, x22, [x0, 16 * 1]
    ldp x23, x24, [x0, 16 * 2]
    ldp x25, x26, [x0, 16 * 3]
    ldp x27, x28, [x0, 16 * 4]
    ldp fp, lr, [x0, 16 * 5]
    ldr x9, [x0, 16 * 6]
    mov sp, x9

    mov x0, x3

    dsb ish
    msr ttbr0_el1, x4
    tlbi vmalle1is
    dsb ish
    isb

    msr elr_el1, x1
    msr spsr_el1, x2
    eret

.global get_current
get_current:
    mrs x0, tpidr_el1
    ret

.global set_current
set_current:
    msr tpidr_el1, x0
    ret

.global exec_prog
exec_prog:
    dsb ish
    msr ttbr0_el1, x2
    tlbi vmalle1is
    dsb ish
    isb

    msr spsr_el1, xzr
    msr elr_el1, x0
    msr sp_el0, x1
    eret