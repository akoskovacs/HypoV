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
    . = 16M;
    .text : ALIGN(4K)
    {
        *(.text)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
    }

    .data : ALIGN(4K)
    {
        *(.data)
    }

    .note :
    {
        *(.note.gnu.build-id)
    }

    .bss : ALIGN(4K)
    {
        *(.bss)
    }

    .common : ALIGN(4K)
    {
        *(.common)
    }
}
