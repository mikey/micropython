SECTIONS
{
 _start = .;
 . = 0;
 .head : {
  KEEP(*(.head))
 }
  . = 0x10000;
  .text : { *(.text) }
  . = 0x50000;
  .data : { *(.data) }
  .bss : { *(.bss) }
  . = 0x80000; /* ALSO CHANGE THIS IN HEAD.S */
  .stack : { *(.stack) }
}
