; +------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2018                           |
; |                                                            |
; | Low-level CPU stuff                                        |
; +------------------------------------------------------------+

global __cpu_gdt64_init
global __cpu_long_mode_enter
global __cpu_compat_mode_disable

%define CR0_PG_BIT 31

%define DESC_PRESENT    (1 << 15)
%define DESC_DATA       (1 << 12); bit 11 = 0
%define DESC_CODE       (1 << 12)|(1 << 11)
%define LONG_MODE       (1 << 21)
%define DESC_TSS        (1 << 8)|(1 << 11)


align 4
section .text
bits 32
__cpu_long_mode_enter:
;   xchg bx, bx         ; Bochs breakpoint, if needed
    mov eax, cr0        ; Read CR0
    bts eax, CR0_PG_BIT ; Set PG bit
    mov cr0, eax
    jmp .arch64         ; Should be in 64bit
.arch64:
    ret
; Should be unreachable
.halt:
    hlt
    jmp .halt
; End of __cpu_enter_long_mode

; XXX: Must be already in 64 bit mode
__cpu_gdt64_init:
    ; Setup GDTR
    lgdt [gdt_segment_64]
    ; Jump with the new CS segment selector
    jmp 0x8:.reload_segments
; Set up the other segments too
.reload_segments:
    xor eax, eax
    mov ss, ax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    ret
; End of __cpu_init_gdt64

; Disable compatilibty mode, enable true 64 bit mode
; XXX: The 64 bit GDT has to be already present
__cpu_compat_mode_disable:
    ; Get the current value
    mov eax, [gdt_64_seg_cs]
    ; Enable LONG_MODE bit in the CS segment
    or eax, LONG_MODE
    ; Store the segment
    mov [gdt_64_seg_cs], eax
    ; Force-reload the old GDT
    lgdt [gdt_segment_64]
ret

section .data
; Unfortunately, a valid 64bit GDT is still needed.
; It is easier to set that up here, in compatilibty mode.
; The CS.L bit could be set later.
align 16
gdt_64:
; Null entry [0x0]
    dd 0x0 ; limit, base address
    dd 0x0 ; limit, base address and flags
; CS entry [0x8]
    ; CPL=0, L=0, CS descriptor, present
gdt_64_seg_cs:
    dd DESC_PRESENT | DESC_CODE ; | LONG_MODE
    dd 0x0 ; no checking on the CS segment
; SS, DS, ES, FS, GS [0x10]
    ; CPL=0, L=1, P=1
    dd DESC_PRESENT | DESC_DATA 
    dd 0x0 ; checking only for FS, GS (maybe used later)
; 64 bit Task State Segment
    ; TSS TODO
; 64 bit Interrupt Descriptor
    ; IDT TODO

align 16
gdt_segment_64:
    dw gdt_segment_64 - gdt_64 - 1
; The absolute address of the GDT has to be
; calculated at runtime, because it could be
; anywhere after relocation
gdt_addr_64:
    dq 0x00

; Dummy LDT
align 16
ldt_segment_64:
    dw 0x00
    dq 0x00