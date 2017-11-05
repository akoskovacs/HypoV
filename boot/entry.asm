; +------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                           |
; |                                                            |
; | First code linked to the main binary (32-bit code)         |
; +------------------------------------------------------------+

global hv_multiboot_entry
extern hv_entry

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
hv_multiboot_entry:
    call hv_entry
    jmp $