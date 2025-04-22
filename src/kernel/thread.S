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
    stp x1, x2, [x0, 16 * 5]
    str x3, [x0, 16 * 6]

    ret

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
  msr spsr_el1, xzr
  msr elr_el1, x0
  msr sp_el0, x1
  eret