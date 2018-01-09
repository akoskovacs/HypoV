; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Basic assembly entry and setup functionality                        |
; +---------------------------------------------------------------------+

global hv_entry_64
extern hv_start

section .text
bits 64
hv_entry_64:
    xchg bx, bx ; Bochs
    lea rsp, [rel hv_stack_64+CONFIG_SZ_HV_STACK]
    call hv_start
; Should be unreachable
.halt:
    hlt
    jmp .halt

section .bss
hv_stack_64: resb CONFIG_SZ_HV_STACK
