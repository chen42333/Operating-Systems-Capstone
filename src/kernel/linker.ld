ENTRY(_start)

v_kernel_space = 0xffff000000000000;

MEMORY {
  KERNEL(rwx) : ORIGIN = v_kernel_space + 0x80000, LENGTH = 512K
}
SECTIONS {
  .text ALIGN(8): { 
    _stext = .;
    KEEP(*(.text._start))
    *(.text)
    _etext = .;
  } > KERNEL
  .rodata ALIGN(8): { 
    _srodata = .;
    *(.rodata)
    _erodata = .;
  } > KERNEL
  .data ALIGN(8): { 
    _sdata = .;
    *(.data) 
    _edata = .;
  } > KERNEL
  .bss ALIGN(8): { 
    _sbss = .;
    *(.bss) 
    _ebss = .;
  } > KERNEL

  _estack = v_kernel_space + 0x100000;
  _sbrk = v_kernel_space + 0x10000000;
}