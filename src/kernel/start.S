.equ WORD_SIZE, 8

.section .bss
.align 3
.global var
var: .skip 8 

.section .text._start
.global _start

_start:
  mov x28, x0 // .dtb address
  bl from_el2_to_el1
  bl set_exception_vector_table
  mrs x0, MPIDR_EL1
  and x0, x0, #0xff
  cbz x0, _core_0

_other_cores:
  wfe
  b _other_cores
  
_core_0:
  // Initialize .bss
  ldr x0, =_sbss
  ldr x1, =_ebss

_loop1:
  cmp x0, x1
  bge _end_loop1
  str xzr, [x0]
  add x0, x0, WORD_SIZE
  b _loop1

_end_loop1:
  ldr x0, =_estack  // Set stack pointer
  mov sp, x0
  mov x0, x28
  b main