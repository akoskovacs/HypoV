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
    . = 0x1000;
    .multiboot : AT(0x1000)
    {
        *(.multiboot.header)
    }

    . = 1M;
    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot.text)
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
