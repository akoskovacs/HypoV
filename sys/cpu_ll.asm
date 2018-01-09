; +------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2018                           |
; |                                                            |
; | Low-level CPU stuff                                        |
; +------------------------------------------------------------+

global __cpu_enter_long_mode
global __cpu_call_64

%define CR0_PG_BIT 31

align 4
section .text
bits 32
__cpu_enter_long_mode:
;   xchg bx, bx         ; Bochs breakpoint, if needed
    mov eax, cr0        ; Read CR0
    bts eax, CR0_PG_BIT ; Set PG bit
    mov cr0, eax
    jmp .arch64         ; Should be in 64bit
.arch64:
    ret

__cpu_call_64:
    xchg bx, bx         ; Bochs breakpoint, if needed
    pop edi
    pop eax
    call eax
    ret
    ; call [qword rax]
    ; db 0xff
    ; db 0xd0