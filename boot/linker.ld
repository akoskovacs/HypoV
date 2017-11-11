/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Main linker script (designed for multiboot)                |
 * +------------------------------------------------------------+
*/
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY (hv_multiboot_entry)
/* page_size = 4096;
hhalf = 0; */

SECTIONS
{
    /* This stage the paging is still not enabled.
     * (physical = virtual)
     *  . = 0x00100000;
     */

    . = 1M;
    .multiboot BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot.header)
        *(.multiboot.text)
    }
    
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }
    
    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(.COMMON)
        *(.bss)
    }
    .note ALIGN(4K) :
    {
        *(.note.gnu.build-id)
    }
}
