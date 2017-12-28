; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | The multiboot header and entry point for the bootloader             |
; |                                                                     |
; | Multiboot specification:                                            |
; |   https://www.gnu.org/software/grub/manual/multiboot/multiboot.html |
; +---------------------------------------------------------------------+

global hv_multiboot_entry
global hv_multiboot_header
extern hv_entry
extern sys_chainload

; Multiboot definitions
%define MB_ALIGN        0x001
%define MB_MEMINFO      0x002
%define MB_FLAGS        (MB_ALIGN | MB_MEMINFO)

%define MB_MAGIC        0x1BADB002
%define MB_CHECKSUM     ~(MB_MAGIC + MB_FLAGS)+1

%define MB_LDR_MAGIC    0x2BADB002

; Multiboot header start
align 4
section .multiboot.header
multiboot_header_start:
    dd MB_MAGIC
    dd MB_FLAGS
    dd MB_CHECKSUM
    dd 0 ; Header address
    dd 0 ; Loader address
    dd 0 ; Loader address end
    dd 0 ; BSS address end
    dd 0 ; Entry-point address
    dd 0 ; Mode type

section .multiboot.text
bits 32
align 4
; The entry point for the multiboot bootloader
hv_multiboot_entry:
; Bochs breakpoint
    xchg bx, bx
; Reset EFLAGS
    push 0
    popf
; The stack grows down from the reserved area
    mov esp, hv_stack+CONFIG_STACK_SIZE
    mov ebp, esp
; If the magic number is wrong, we can't trust the bootloader 
; anymore. Abort booting and try to chainload something else.
    cmp eax, MB_LDR_MAGIC
    jne .panic_chainload
; Pass struct MultiBootInfo * on the stack
    push ebx
; Call the 32 bit C entry function from sys/init.c
; hv_entry(struct MultiBootInfo *)
    call hv_entry
.panic_chainload:
    call sys_chainload
; Something went wrong somewhere
.hang:
    jmp .hang

section .bss
hv_stack: resb CONFIG_STACK_SIZE
