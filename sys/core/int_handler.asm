; +---------------------------------------------------------------------+
; | Copyright (C) Kovács Ákos - 2018                                    |
; |                                                                     |
; | Global assembly interrupt handler. Called by all auto-generated     |
; | trap handlers from assembly, with their respective number in stack. |
; +---------------------------------------------------------------------+
[bits 64]
global __int_handler
extern hv_handle_interrupt

section .text
; Set-up trapframe
__int_handler:
    ;Debug guard
    ;xchg bx, bx
    ;push 0x4242424242
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rdx
    push rcx
    push rbx
    push rax

; Make the trapframe the first argument
    mov rdi, rsp
    call hv_handle_interrupt

; Clean-up the interrupt stack
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rbp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    add rsp, 16
iretq
