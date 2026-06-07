/*
 * Automatically generated C config: don't edit
 * HypoV Configuration
 */
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
    /* Load at 128MB — above where the Linux guest kernel decompresses
     * (typically 0x1000000 = 16MB), so the guest can't corrupt our
     * NPT page tables which live in .bss near this base address. */
    . = 128M;
    __hvcore_start = .;
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
    . = ALIGN(4K);
    __hvcore_end = .;
}
