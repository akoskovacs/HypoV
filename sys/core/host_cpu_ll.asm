; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Low-level host CPU utility assembly functions                       |
; +---------------------------------------------------------------------+

#include <gdt.h>

global interrupt_handler
global __gdt_setup_64
global __tss_setup_64
global __idt_setup_64

section .text
interrupt_handler:
    ;pushad
    ; call int_hdl
    ;popad
    iret

; void __gdt_setup_64(uint16_t seglimit, unsigned long base)
; seglimit : di
; base : rsi
__gdt_setup_64:
    mov [rel gdt_segment_limit], di
    mov [rel gdt_segment_base], rsi
    lgdt [rel gdt_segment_limit]
;    jmp far GDT_SEL(GDT_HV_CODE64):.segment_setup
    jmp .segment_setup
.segment_setup:
    ; The other segments are ignored in 64 bit mode
    mov ax, GDT_SEL(GDT_HV_DATA64)
    mov fs, ax
    mov gs, ax
ret

; Load 32 bit Task State Register, from the GDT
; void __tss_setup_64(uint16_t tss_selector);
__tss_setup_64:
    mov [rel tss_selector_64], di
    lea rax, [rel tss_selector_64]
    mov [rax], di
    ltr [rax]
ret

__idt_setup_64:
ret

section .data
align 8
gdt_segment_limit:
    dw 0x0
gdt_segment_base:
    dd 0x0
    dd 0x0

align 8
; TSS selector in the GDT
tss_selector_64:
    dw 0x0
