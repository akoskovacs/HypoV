/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Main linker script (designed for multiboot)                |
 * +------------------------------------------------------------+
*/

/* Multiboot compatible bootloaders can load ELF binaries
 * natively. The entry point will be filled in by the
 * linker to the appropriate assembly code in boot/entry.asm
*/
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY (hv_multiboot_entry)

SECTIONS
{
    /* Help the bootloader to find the multiboot header
     * by putting it before the 8KB mark.
    */
    . = 4K;
    .multiboot : AT(0x1000)
    {
        *(.multiboot.header)
    }

    /* If everything is fine, the bootloader will
     * load the 32bit protected mode code after the 
     * the first 1M of the memory.
    */
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
