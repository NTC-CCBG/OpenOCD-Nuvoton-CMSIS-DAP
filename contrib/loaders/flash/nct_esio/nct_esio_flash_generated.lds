


MEMORY {

 BUFFER (RWX) : ORIGIN = 0x200C0000,
   LENGTH = 0x1000 + 20

 PROGRAM (RWX) : ORIGIN = 0x200C1014,
    LENGTH = 0x1000
}


SECTIONS
{
 .buffers (NOLOAD) :
 {
  _buffers = .;
  *(.buffers.g_cfg)
  *(.buffers.g_buf)
  *(.buffers*)
  _ebuffers = .;
 } > BUFFER

 .text :
 {
  _text = .;
  *(.entry*)
  *(.text*)
  _etext = .;
 } > PROGRAM

 .data :
 { _data = .;
  *(.rodata*)
  *(.data*)
  _edata = .;
 } > PROGRAM

 .bss :
 {
  __bss_start__ = .;
  _bss = .;
  *(.bss*)
  *(COMMON)
  _ebss = .;
  __bss_end__ = .;
 } > PROGRAM

 .stack (NOLOAD) :
 {
  _stack = .;
  *(.stack*)
  _estack = .;
 } > PROGRAM
}
