; +------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                           |
; |                                                            |
; | First code linked to the main binary (32-bit code)         |
; +------------------------------------------------------------+

global hv_multiboot_entry
extern hv_entry

section .multiboot_header

bits 32
hv_multiboot_entry:
    call hv_entry
