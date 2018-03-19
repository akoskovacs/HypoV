; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2017                                    |
; |                                                                     |
; | Basic assembly entry and setup functionality                        |
; +---------------------------------------------------------------------+

#include <gdt.h>

global hv_entry_64
extern hv_start
extern os_error_stub

section .text
bits 64
hv_entry_64:
    xchg bx, bx ; Bochs
    lea rsp, [rel hv_stack_64+CONFIG_SZ_HV_STACK]
    ; Operating systems usually clear the RDI register
    cmp rdi, 0
    je os_error_stub
    ; Load up the 64 bit Task State Segment
    ;ltr [rel tss_selector]
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
