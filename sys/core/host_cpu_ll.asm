; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2018                                    |
; |                                                                     |
; | Low-level host CPU utility assembly functions                       |
; +---------------------------------------------------------------------+

#include <gdt.h>

global __get_relocation_offset
global __gdt_setup_64
global __tss_setup_64
global __idt_setup_64

section .text

; Get the offset of the current relocation for all symbols
; unsigned long __get_relocation_offset(void)
__get_relocation_offset:
    lea rax, [rel __get_relocation_offset] ; Current relative offset
    sub rax, __get_relocation_offset       ; Substract the absolute address, to get offset
ret

; Get the current value of RSP at the calling point
; unsigned long __get_rsp(void)
__get_rsp:
    mov rax, rsp
    sub rax, 8     ; No need for the return address
ret

; void __gdt_setup_64(uint16_t seglimit, unsigned long base)
; seglimit : di
; base : rsi
__gdt_setup_64:
;    xchg bx, bx ; Bochs
    mov [rel gdt_segment_limit], di
    mov [rel gdt_segment_base], rsi
    lgdt [rel gdt_segment_limit]
    ; Far return to reload CS with the new code64 selector
    lea rax, [rel gdt64_seg_done]
    push GDT_SEL(GDT_HV_CODE64)
    push rax
    db 0x48, 0xCB ; REX.W retf (retfq)
gdt64_seg_done:
    mov ax, GDT_SEL(GDT_HV_DATA64)
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
ret

; Load 64 bit Task State Register, from the GDT
; void __tss_setup_64(uint16_t tss_selector);
__tss_setup_64:
;    xchg bx, bx
    mov [rel tss_selector_64], di
    lea rax, [rel tss_selector_64]
    ltr [rax]
ret

; void __idt_setup_64(uint16_t idt_limit, uint64_t base)
__idt_setup_64:
    mov [rel idt_selector], di
    mov [rel idt_segment_base], rsi
    lidt [rel idt_selector]
    xchg bx, bx
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

align 8
idt_selector:
    dw 0x0
idt_segment_base:
    dq 0x0

section .note.GNU-stack noalloc noexec nowrite progbits
