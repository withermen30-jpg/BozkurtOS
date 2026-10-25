OUTPUT_FORMAT(elf64-x86-64)
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .text : { *(.text.start) *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    . = ALIGN(16);
    bss_start = .;
    .bss : { *(.bss*) *(COMMON) }
    bss_end = .;
    /DISCARD/ : { *(.comment) *(.note*) *(.eh_frame*) }
}
