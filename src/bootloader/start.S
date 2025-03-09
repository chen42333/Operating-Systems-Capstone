.equ WORD_SIZE, 8
.equ TARGET_ADDR, 0x60000

.section .data
.align 3
_reloc: .word 0

.section .text._start
.global _start

_start:
  adr x1, _reloc
  ldr x2, [x1] 
  cmp x2, #0
  bne _start_new_loc
  mov w4, 1
  str w4, [x1]

  // Relocation
  mov x1, TARGET_ADDR // Target start address
  mov x2, x1
  adr x3, _stext // Start address
  adr x4, _edata // End address

_loop1:
  cmp x3, x4
  bge _end_loop1
  ldr x5, [x3]
  str x5, [x2]
  add x2, x2, WORD_SIZE
  add x3, x3, WORD_SIZE
  b _loop1

_end_loop1:
  br x1

_start_new_loc:
  // Initialize .bss
  adr x1, _sbss
  adr x2, _ebss

_loop2:
  cmp x1, x2
  bge _end_loop2
  str xzr, [x1]
  add x1, x1, WORD_SIZE
  b _loop2

_end_loop2:
  adr x1, _estack  // Set stack pointer
  mov sp, x1
  b main
  