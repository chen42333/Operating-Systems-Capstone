.equ CORE0_IRQ_SOURCE, 0x40000060 + v_kernel_space
.equ AUX_MU_IIR_REG, 0x3f215048 + v_kernel_space
.equ EC_MASK, 0xfc000000
.equ EC_SVC, 0b010101

.section .text

// save general registers to stack
.macro save_all
    sub sp, sp, 34 * 8
    stp x0, x1, [sp ,16 * 0]
    stp x2, x3, [sp ,16 * 1]
    stp x4, x5, [sp ,16 * 2]
    stp x6, x7, [sp ,16 * 3]
    stp x8, x9, [sp ,16 * 4]
    stp x10, x11, [sp ,16 * 5]
    stp x12, x13, [sp ,16 * 6]
    stp x14, x15, [sp ,16 * 7]
    stp x16, x17, [sp ,16 * 8]
    stp x18, x19, [sp ,16 * 9]
    stp x20, x21, [sp ,16 * 10]
    stp x22, x23, [sp ,16 * 11]
    stp x24, x25, [sp ,16 * 12]
    stp x26, x27, [sp ,16 * 13]
    stp x28, x29, [sp ,16 * 14]
    str x30, [sp, 16 * 15]

    mrs x0, elr_el1
    mrs x1, spsr_el1
    stp x0, x1, [sp ,16 * 16]
.endm

// load general registers from stack
.macro load_all
    ldp x0, x1, [sp ,16 * 16] // restore elr_eln and spsr_eln first
    msr elr_el1, x0
    msr spsr_el1, x1

    ldp x0, x1, [sp ,16 * 0]
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 34 * 8
.endm

.macro load_all_wo_x0
    mov x2, x0
    ldp x0, x1, [sp ,16 * 16] // restore elr_eln and spsr_eln first
    msr elr_el1, x0
    msr spsr_el1, x1

    ldp x0, x1, [sp ,16 * 0]
    mov x0, x2
    ldp x2, x3, [sp ,16 * 1]
    ldp x4, x5, [sp ,16 * 2]
    ldp x6, x7, [sp ,16 * 3]
    ldp x8, x9, [sp ,16 * 4]
    ldp x10, x11, [sp ,16 * 5]
    ldp x12, x13, [sp ,16 * 6]
    ldp x14, x15, [sp ,16 * 7]
    ldp x16, x17, [sp ,16 * 8]
    ldp x18, x19, [sp ,16 * 9]
    ldp x20, x21, [sp ,16 * 10]
    ldp x22, x23, [sp ,16 * 11]
    ldp x24, x25, [sp ,16 * 12]
    ldp x26, x27, [sp ,16 * 13]
    ldp x28, x29, [sp ,16 * 14]
    ldr x30, [sp, 16 * 15]
    add sp, sp, 34 * 8
.endm

.macro inc_irq_nested_count
    adr x1, irq_nested_count
    ldr w2, [x1]
    add w2, w2, 1
    str w2, [x1]
.endm

.macro dec_irq_nested_count
    adr x1, irq_nested_count
    ldr w2, [x1]
    sub w2, w2, 1
    str w2, [x1]
.endm

.global from_el2_to_el1
from_el2_to_el1:
    mov x0, (1 << 31) // EL1 uses aarch64
    msr hcr_el2, x0
    mov x0, 0x3c5 // EL1h (SPSel = 1) with interrupt disabled
    msr spsr_el2, x0
    msr elr_el2, lr
    mrs x0, sctlr_el1
    orr x0, x0, 0b11000 // SP alignment check enable for EL1 and EL0
    msr sctlr_el1, x0
    eret // return to EL1

empty:
    eret

exception_handler:
    save_all
    inc_irq_nested_count
exception:
    bl exception_entry
    dec_irq_nested_count
    load_all
    eret

syscall_handler:
    save_all
    inc_irq_nested_count
    mrs x0, esr_el1
    and x0, x0, EC_MASK
    lsr x0, x0, #26
    cmp x0, EC_SVC
    bne exception // Not system call
    mov x0, sp
    bl syscall_entry
    dec_irq_nested_count
    load_all_wo_x0
    eret

irq_handler:
    save_all
    inc_irq_nested_count
    ldr x0, =CORE0_IRQ_SOURCE
    ldr x1, [x0]
    and x2, x1, 0b10
    cbnz x2, core_timer_handler // timer interrupt

    ldr x0, =AUX_MU_IIR_REG
    ldr x1, [x0]
    and x2, x1, 0b110
    cmp x2, 0b10
    beq tx_int_handler

    cmp x2, 0b100
    beq rx_int_handler

    b irq_handler_end

core_timer_handler:
    bl timer_int
    b irq_handler_end

tx_int_handler:
    bl tx_int
    b irq_handler_end

rx_int_handler:
    bl rx_int
    b irq_handler_end

irq_handler_end:
    dec_irq_nested_count
    load_all
    eret

.align 11 // vector table should be aligned to 0x800
.global exception_vector_table
exception_vector_table:
  b empty // branch to a handler function.
  .align 7 // entry size is 0x80, .align will pad 0
  b empty
  .align 7
  b empty
  .align 7
  b empty
  .align 7

  b exception_handler
  .align 7
  b irq_handler
  .align 7
  b empty
  .align 7
  b empty
  .align 7

  b syscall_handler
  .align 7
  b irq_handler
  .align 7
  b empty
  .align 7
  b empty
  .align 7

  b empty
  .align 7
  b empty
  .align 7
  b empty
  .align 7
  b empty
  .align 7

.global set_exception_vector_table
set_exception_vector_table:
  adr x0, exception_vector_table
  msr vbar_el1, x0
  ret