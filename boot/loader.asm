; +------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                           |
; |                                                            |
; | Real-mode 16-bit bootloader of the installer               |
; +------------------------------------------------------------+

BITS 16

%define FILLER       0x90    ; NOP instruction
%define LOAD_ADDRESS 0x7c00  ; Default BIOS load address

; org 0x7c00                 ; Done in linker script

loader_entry:
    xor ax, ax
    mov ss, ax
    mov sp, LOAD_ADDRESS
    mov es, ax
    mov ds, ax
    mov si, LOAD_ADDRESS
    mov di, 0x600
    cld
    rep movsb
    jmp $

times 510-($-$$) db 0x90
db 0x55, 0xAA                ; BIOS signature