; +------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2017                           |
; |                                                            |
; | Real-mode 16-bit bootloader of the installer               |
; +------------------------------------------------------------+

bits 16

%define FILLER              0x00    ; Zero bytes
%define BIOS_LOAD_ADDRESS   0x7c00  ; Default BIOS load address
%define FAT32_OFFSET        0x5A    ; FAT32 offset
%define LOADER_MAX_SIZE     422     ; Maximum size of the bootloader code&data
%define SCREEN_LINES        40
%define SCREEN_WIDTH        80

%define LOAD_ADDRESS       BIOS_LOAD_ADDRESS + FAT32_OFFSET

org LOAD_ADDRESS

loader_entry:
    cli
    cld
    xor ax, ax
    mov es, ax
    mov ds, ax
    mov fs, ax
    mov bx, ax

    mov ah, 0x0e
    mov al, 0x0a
    mov cx, SCREEN_LINES
    rep int 0x10

.print:
    mov al, [hello_world+bx]
    cmp al, 0
    je nothing_to_do
    int 0x10
    inc bx
    jmp short .print

nothing_to_do:
    hlt
    
hello_world db 'HypoV Bootloader v0.1', 0x0d, 0x0a,      \
               'Copyright Akos Kov', 0xa0, 'cs', 0x0d, 0x0a,  \
               ' ', 0x0d, 0x0a,                          \
               'Hello, vilag!', 0x0d, 0x0a, 0x0          \

other_stuff:
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


times LOADER_MAX_SIZE-($-$$)-2 db FILLER
dw 0xaa55                ; BIOS signature