.equ WORD_SIZE, 8

.section .bss
.align 3
.global var
var: .skip 8 

.section .text._start
.global _start, exec_prog

_start:
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
  adr x0, _sbss
  adr x1, _ebss

_loop1:
  cmp x0, x1
  bge _end_loop1
  str xzr, [x0]
  add x0, x0, #WORD_SIZE
  b _loop1

_end_loop1:
  adr x0, _estack  // Set stack pointer
  mov sp, x0
  b main
  
exec_prog:
  mov x0, 0x3c0 
  msr spsr_el1, x0
  adr x0, _sprog
  msr elr_el1, x0
  adr x0, _eprog_stack
  msr sp_el0, x0
  eret