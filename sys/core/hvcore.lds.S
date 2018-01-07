/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Linker script for the loadable 64bit ELF64                 |
 * +------------------------------------------------------------+
*/

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)

ENTRY(hv_entry_64)

SECTIONS 
{
    . = 0x1000;
    .text : ALIGN(2M)
    {
        *(.text)
    }

    .data : ALIGN(2M)
    {
        *(.data)
    }

    .rodata : ALIGN(2M)
    {
        *(.rodata)
    }

    .note :
    {
        *(.note.gnu.build-id)
    }

    .bss : ALIGN(2M)
    {
        *(.bss)
    }

    .common : ALIGN(4K)
    {
        *(.common)
    }
}
