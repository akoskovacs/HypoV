; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Abort booting into the hypervisor and resume booting from the fist  |
; | hard disk. To make this possible the code has to exit from          |
; | protected mode, then read the first sector using BIOS from 16 bit   |                                                                   |
; | real mode.                                                          |
; +---------------------------------------------------------------------+

#include <gdt.h>

global sys_chainload

section .chainloader.text
align 4

bits 32
prot_disable:
    mov eax, cr0
    xor eax, 0x1 
    mov cr0, eax
    nop
    nop
    nop
    nop
ret

resetpic:                                  ; reset 8259 master and slave pic vectors
    push ax                                ; expects bh = master vector, bl = slave vector
    mov  al, 0x11                          ; 0x11 = ICW1_INIT | ICW1_ICW4
    out  0x20, al                          ; send ICW1 to master pic
    out  0xA0, al                          ; send ICW1 to slave pic
    mov  al, bh                            ; get master pic vector param
    out  0x21, al                          ; send ICW2 aka vector to master pic
    mov  al, bl                            ; get slave pic vector param
    out  0xA1, al                          ; send ICW2 aka vector to slave pic
    mov  al, 0x04                          ; 0x04 = set slave to IRQ2
    out  0x21, al                          ; send ICW3 to master pic
    shr  al, 1                             ; 0x02 = tell slave its on IRQ2 of master
    out  0xA1, al                          ; send ICW3 to slave pic
    shr  al, 1                             ; 0x01 = ICW4_8086
    out  0x21, al                          ; send ICW4 to master pic
    out  0xA1, al                          ; send ICW4 to slave pic
    pop  ax                                ; restore ax from stack
ret                                        ; return to caller

sys_chainload:
    cli
; Bochs breakpoint
    xchg bx, bx
    lgdt [gdt_selector_16]
    ; Jump back to 16 bit protected real mode
    jmp far 0x8:.sys_prot_realmode
bits 16
.sys_prot_realmode:
    nop
    nop
    nop
    nop
    ; Load 16 bit data segments
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; Disable protected mode
    mov eax, cr0
    xor eax, 0x1 
    mov cr0, eax
    nop
    nop
    nop
    nop
;    call prot_disable
    mov sp, 0x8000
    ; The GDT segment selectors are not valid anymore
#if 1
    jmp 0x0:.sys_realmode
.sys_realmode:
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
#endif
    ; Enable interrupts
    mov bx, 0x0870
    call resetpic
    ; Load 16 bit BIOS interupt table
    lidt [cs:idt_16]
    sti
reset_disk:
    ; Reset disk controller
    mov ah, 0x00 ; reset function
    mov dl, 0x80 ; 1. hard disk drive
    int 0x13
    call prot_disable
    xchg bx, bx
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
    call prot_disable
    jc .read_disk
    ; The boot sector might depend on the
    ; correct drive number
    mov dl, 0x80
    xor ax, ax
    ;mov cs, ax
    jmp far 0x0:0x7c00

align 4
idt_16:
;    dw 0x3FF ; Limit
    dw 0x3FF ; Limit
    dd 0x0   ; Base

align 4
tr_16:
;    dw 0x3FF ; Limit
    dw 0xFFFF ; Limit
    dd 0x0   ; Base

align 4
gdt_selector_16:
    dw 3*8-1
    dw gdt_16

align 4
gdt_16:
; Null descriptor
   dw 0x0, 0x0
   db 0x0, 0x0, 0x0, 0x0
; 16 bit code (0x8)
   dw 0xFFFF, 0x0
   db 0x0, 0x9E, 0x0, 0x0
; 16 bit data (0x10)
   dw 0xFFFF, 0x0
   db 0x0, 0x92, 0x0, 0x0
