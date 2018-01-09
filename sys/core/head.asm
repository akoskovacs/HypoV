; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Basic assembly entry and setup functionality                        |
; +---------------------------------------------------------------------+

global hv_entry_64

extern hv_start

%define DESC_PRESENT    (1 << 15)
%define DESC_DATA       (1 << 12); bit 11 = 0
%define DESC_CODE       (1 << 12)|(1 << 11)
%define LONG_MODE       (1 << 21)
%define DESC_TSS        (1 << 8)|(1 << 11)

;section .bss
;tss_64:
    
;section .rodata

section .text
bits 32
hv_entry_64:
    nop 
    nop
    nop
    nop
    xchg bx, bx ; Bochs
; Set up stack
    lea esp, [rel hv_stack_64+CONFIG_SZ_HV_STACK]
    lea ebx, [rel gdt_64]
    lea ecx, [rel gdt_addr]
; Copy the absolute address of GDT
    mov [ecx], ebx
; Setup GDTR
    lea eax, [rel gdt_segment_64]
    lgdt [eax]
; Jump with the new CS segment selector
    jmp 0x8:segment_setup
;    push 0
;    push 0x8 ; CS
;    lea eax, [rel segment_setup]
;    push eax
;    ret
;    lea rax, [rel ldt_segment_64]
;    lidt [rax]
bits 64
segment_setup:
    xor eax, eax
;    mov ss, ax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    call hv_start
    
;    lgdt [rel gdt_segment_64]
;    mov eax, 0x08
;    mov cs, ax
;    jmp 0x08:.reload_segments
.reload_segments:
    mov eax, 0x10
;    mov ss, ax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax

; Should be unreachable
.halt:
    hlt
    jmp .halt

;section .data
; Unfortunately, a valid 64bit GDT is still needed
align 16
gdt_64:
; Null entry [0x0]
    dd 0x0 ; limit, base address
    dd 0x0 ; limit, base address and flags
; CS entry [0x8]
    ; CPL=0, L=1, CS descriptor, present
    dd DESC_PRESENT | LONG_MODE | DESC_CODE
    dd 0x0 ; no checking on the CS segment
; SS, DS, ES, FS, GS [0x10]
    ; CPL=0, L=1, P=1
    dd DESC_PRESENT | DESC_DATA 
    dd 0x0 ; checking only for FS, GS (maybe used later)
; 64 bit Task State Segment
    ; TSS TODO

gdt_segment_64:
    dw gdt_segment_64 - gdt_64 - 1
; The absolute address of the GDT has to be
; calculated at runtime, because it could be
; anywhere after relocation
gdt_addr:
    dq 0x00

; Dummy LDT
align 16
ldt_segment_64:
    dw 0x00
    dq 0x00

hv_start_addr:
    dq 0x0

section .rodata
my_name_is:
    db 'Akos Kovacs', 0

section .bss
hv_stack_64: resb CONFIG_SZ_HV_STACK
