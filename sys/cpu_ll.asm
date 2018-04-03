; +------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2018                           |
; |                                                            |
; | Low-level CPU stuff                                        |
; +------------------------------------------------------------+


global __gdt_setup_32
global __tss_setup_32
global __cpu_long_mode_enter
global __cpu_call_64

#include <gdt.h>

align 4
section .text
bits 32
; Puts the CPU into 64 bit Compatibility Mode
; XXX: Valid page tables and CPU bits have to be set up
;
; void __cpu_long_mode_enter(void);
__cpu_long_mode_enter:
;   xchg bx, bx         ; Bochs breakpoint, if needed
    mov eax, cr0        ; Read CR0
    bts eax, CR0_PG_BIT ; Set PG bit
    mov cr0, eax
    jmp .arch64         ; Should be in 64bit compatiblity mode
.arch64:                ; an immidiate branch is needed by the spec
    ret
; Should be unreachable
.halt:
    hlt
    jmp .halt
; End of __cpu_enter_long_mode

; Call in to the final 64 bit code, by setting up
; 32- and 64 bit code and data selector in the 
; new Global Descriptor Table. The IDT will be cleared.

; XXX: Must be already in 64 bit compatibility mode
;
; void __noreturn __cpu_call_64(uint32_t jmp_addr, uint32_t arg0);
__cpu_call_64:
    xchg bx, bx
    ; This function will not return, no need for 
    ; subroutine prologues, just get the first two
    ; parameters (uint32_t jmp_addr, uint32_t arg0)
    mov eax, [esp + 4]  ; jmp_addr
    ; The second parameter uses the 64bit 
    ; calling convention. The function parameter 
    ; will appear in the RDI register.
    mov edi, [esp + 8]  ; arg0
    ; Modify target on-the-fly
    mov dword [.jmp_addr], eax
    ; Use the new 64 bit segments
    mov eax, GDT_SEL(GDT_SYS_DATA64)
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ; Self-modifying machine code to jump directly to the
    ; desired 64bit code.
    db 0xEA ; jmp far cs:whatever
.jmp_addr:
    dd 0x0  ; Address from EAX
    dw GDT_SEL(GDT_SYS_CODE64) ; 64 bit CS selector
ret ; __noreturn
; End of __cpu_call_64

; void __gdt_setup_32(unsigned long seglimit, unsigned long base)
__gdt_setup_32:
    mov eax, [esp + 4]  ; seglimit
    mov [gdt_segment_limit], eax
    mov eax, [esp + 8]  ; base
    mov [gdt_segment_base], eax
    lgdt [gdt_segment_limit]
    jmp far GDT_SEL(GDT_SYS_CODE32):.segment_setup
.segment_setup:
    mov ax, GDT_SEL(GDT_SYS_DATA32)
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
ret

; Load 32 bit Task State Register, from the GDT
; void __tss_setup_32(uint16_t tss_selector);
__tss_setup_32:
    mov ax, [esp + 4]
    mov [tss_selector_32], ax
    ltr [tss_selector_32]
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
tss_selector_32:
    dw 0x0
