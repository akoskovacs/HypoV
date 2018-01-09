; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Abort booting into the hypervisor and resume booting from the fist  |
; | hard disk. To make this possible the code has to exit from          |
; | protected mode, then read the first sector using BIOS from 16 bit   |                                                                   |
; | real mode.                                                          |
; +---------------------------------------------------------------------+

global sys_chainload
;global hv_chainload_real

section .chainloader.text
align 4
bits 32
sys_chainload:
    cli
; Bochs breakpoint
    xchg bx, bx
    mov esp, eax
    mov ebp, eax
; Disable the A20 gate
;    mov al, 0x00
;    out 0x92, al ; Fast A20 disable
; Exit from protected mode, back to realmode
    mov eax, cr0
    xor eax, 0x1 
    mov cr0, eax
    jmp word 0x10:sys_chainload_real
    
align 4
bits 16

sys_chainload_real:
; Clean instruction cache
    nop
    nop
    nop
    nop
    ;mov ax, 0x10 ; cs = GDT[2]
    ;mov cs, ax
    mov ax, 0x18 ; cs = GDT[2]
    mov ds, ax  ; ds = GDT[3]
    mov es, ax
    mov fs, ax
    mov gs, ax
    nop
; Disable A20 gate with BIOS
    ;mov ax, 0x2400
    ;int 0x15
; Reset disk controller
reset_disk:
    mov ah, 0x00 ; reset function
    mov dl, 0x80 ; 1. hard disk drive
    int 0x13
    jc reset_disk
; Load the boot sector at 0x0000:0x7c00
    mov bx, 0x0000
    mov es, bx
    mov bx, 0x7c00
.read_disk:
    mov ah, 0x02 ; read function
    mov al, 0x01 ; number of sectors
    mov ch, 0x00 ; cylinder
    mov cl, 0x01 ; sector
    mov dh, 0x00 ; head
    mov dl, 0x80 ; drive
    int 0x13
    jc .read_disk
; The boot sector might depend on the
; correct drive number
    mov dl, 0x80
    xor ax, ax
    ;mov cs, ax
    jmp word 0x00:0x7c00
