ENTRY(_start)
MEMORY
{
  MEM(rwx) : ORIGIN = 0x80000, LENGTH = 64K
}
SECTIONS
{
  .text : { 
    _stext = .;
    *(.text)
    _etext = .;
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

  _estack = 0xf0000;
}