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

%define HV_STACK_SIZE 4096 ; 4Kb stack

; Multiboot definitions
%define MB_ALIGN        0x001
%define MB_MEMINFO      0x002
%define MB_FLAGS        (MB_ALIGN | MB_MEMINFO)

%define MB_MAGIC        0x1BADB002
%define MB_CHECKSUM     ~(MB_MAGIC + MB_FLAGS)+1

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
    mov esp, hv_stack+HV_STACK_SIZE
    mov ebp, esp
; Push the arguments for the C function
    push eax
    push ebx
; Call the 32 bit C entry function in sys/init.c
; hv_entry(struct MultiBootInfo *, int magic)
    call hv_entry
.hang:
    jmp .hang

section .bss
hv_stack: resb HV_STACK_SIZE
