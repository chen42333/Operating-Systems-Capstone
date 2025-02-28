ENTRY(_start)
MEMORY
{
  MEM(rwx) : ORIGIN = 0x60000, LENGTH = 32K
}
SECTIONS
{
  .text : { 
    _stext = .;
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

  _estack = 0x78000;
}