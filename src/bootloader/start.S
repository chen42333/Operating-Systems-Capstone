.equ WORD_SIZE, 8

.section .data
.align 3
.global _skernel
_target_addr: .quad 0x60000   
_skernel: .quad 0x80000 
_reloc: .word 0

.section .text._start
.global _start

_start:
  adr x0, _reloc
  ldr x1, [x0] 
  cmp x1, #0
  bne _start_new_loc
  mov w4, #1
  str w4, [x0]

  // Relocation
  adr x0, _target_addr
  ldr x0, [x0] // Target start address
  mov x1, x0
  adr x2, _stext // Start address
  adr x3, _edata // End address

_loop1:
  cmp x2, x3
  bge _end_loop1
  ldr x4, [x2]
  str x4, [x1]
  add x1, x1, #WORD_SIZE
  add x2, x2, #WORD_SIZE
  b _loop1

_end_loop1:
  br x0

_start_new_loc:
  // Initialize .bss
  adr x0, _sbss
  adr x1, _ebss

_loop2:
  cmp x0, x1
  bge _end_loop2
  str xzr, [x0]
  add x0, x0, #WORD_SIZE
  b _loop2

_end_loop2:
  adr x0, _estack  // Set stack pointer
  mov sp, x0
  b main
  