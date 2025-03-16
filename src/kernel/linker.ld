ENTRY(_start)
MEMORY
{
  MEM(rwx) : ORIGIN = 0x80000, LENGTH = 128K
}
SECTIONS
{
  .text ALIGN(8): { 
    _stext = .;
    KEEP(*(.text._start))
    *(.text)
    _etext = .;
  } > MEM
  .rodata ALIGN(8): { 
    _srodata = .;
    *(.rodata)
    _erodata = .;
  } > MEM
  .data ALIGN(8): { 
    _sdata = .;
    *(.data) 
    _edata = .;
  } > MEM
  .bss ALIGN(8): { 
    _sbss = .;
    *(.bss) 
    _ebss = .;
  } > MEM

  _estack = 0x100000;
  _sprog = 0x100000;
  _eprog_stack = 0x120000;
  _sbrk = 0x180000;
  _ebrk = 0x200000;
}