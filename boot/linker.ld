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
page_size = 4096;
hhalf = 0;

SECTIONS
{
    /* This stage the paging is still not enabled.
     * (physical = virtual)
     */
    . = 0x00100000;
    .multiboot_header :
    {
/*    __start_multiboot_header = .;
        boot/entry.o */
/*    __end_multiboot_header = .; */
    }

    .text : AT(ADDR(.text) - hhalf)
    {
/*    __start_cs_init = .; */
        *(.init.text)
/*    __end_cs_init = .; */
        *(.text)
    }

    .rodata ALIGN(page_size) : AT(ADDR(.rodata) - hhalf)
    {
/*    __start_data_kernel = .; */
        *(.rodata*)
    }

    .data ALIGN(page_size) : AT(ADDR(.data) - hhalf)
    {
        *(.data)
    }

    .bss ALIGN(page_size) : AT(ADDR(.bss) - hhalf)
    {
/*        sbss = .; */
        *(COMMON)
        *(.bss)
/*       ebss = .;
    __end_data_kernel = .; */
    }

    .note ALIGN(page_size) : AT(ADDR(.note) - hhalf)
    {
        *(.note.gnu.build-id)
    }
}
