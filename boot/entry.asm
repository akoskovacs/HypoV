; +------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                           |
; |                                                            |
; | The multiboot header and entry point for the bootloader    |
; +------------------------------------------------------------+

global hv_multiboot_entry
extern hv_entry

%define HV_STACK_SIZE 4096 ; 4Kb stack

; Multiboot definitions
%define MB_ALIGN        0x001
%define MB_MEMINFO      0x002
%define MB_FLAGS        MB_ALIGN | MB_MEMINFO

%define MB_MAGIC        0x1BAD002
%define MB_CHECKSUM     -(MB_MAGIC + MB_FLAGS)

; Multiboot header start
section .multiboot_header
dq MB_MAGIC
dq MB_FLAGS
dq MB_CHECKSUM
dq 0 ; Header address
dq 0 ; Loader address
dq 0 ; Loader address end
dq 0 ; BSS address end
dq 0 ; Entry-point address
dq 0 ; Mode type

bits 32
align 4

; The entry point for the multiboot bootloader
hv_multiboot_entry:
; Bochs breakpoint
    xchg bx, bx
; The stack grows down from the reserved area
    mov esp, hv_stack+HV_STACK_SIZE
    mov ebp, esp
; Push the arguments for the C function
    push eax
    push ebx
; Call the 32 bit C entry function in sys/init.c
; hv_entry(struct MultiBootInfo *, int magic)
    call hv_entry
    jmp $

section .bss
hv_stack: resb HV_STACK_SIZE
