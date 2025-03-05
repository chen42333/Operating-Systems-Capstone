ENTRY(main)
MEMORY
{
  MEM(rwx) : ORIGIN = 0x110000, LENGTH = 128K
}
SECTIONS
{
  .text : { 
    _stext = .;
    KEEP(*(.text._start))
    *(.text)
    _etext = .;
  } > MEM
  .rodata : { 
    _srodata = .;
    *(.rodata)
    _erodata = .;
  } > MEM
  .data : { 
    _sdata = .;
    *(.data) 
    _edata = .;
  } > MEM
  .bss : { 
    _sbss = .;
    *(.bss) 
    _ebss = .;
  } > MEM

  _estack = 0x180000;
}