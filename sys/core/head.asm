; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Basic assembly entry and setup functionality                        |
; +---------------------------------------------------------------------+

#include <gdt.h>

global hv_entry_64
extern hv_start

section .text
bits 64
hv_entry_64:
    xchg bx, bx ; Bochs
    lea rsp, [rel hv_stack_64+CONFIG_SZ_HV_STACK]
    ; Load up the 64 bit Task State Segment
    ;lea rax, [rel tss_selector]
    ltr [rel tss_selector]
    call hv_start
; Should be unreachable
.halt:
    hlt
    jmp .halt

section .rodata
align 8
tss_selector:
    dw GDT_SEL(GDT_SYS_TSS64)

section .bss
hv_stack_64: resb CONFIG_SZ_HV_STACK
