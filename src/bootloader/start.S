.equ WORD_SIZE, 8

.section .text._start
.global _start

_start:
  // Initialize .bss
  adr x0, _sbss
  adr x1, _ebss

_loop:
  cmp x0, x1
  bge _end_loop
  str xzr, [x0]
  add x0, x0, #WORD_SIZE
  b _loop

_end_loop:
  adr x0, _estack  // Set stack pointer
  mov sp, x0
  b main
  