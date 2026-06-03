; +---------------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2026                                    |
; |                                                                     |
; | AMD SVM low-level instruction wrappers                              |
; +---------------------------------------------------------------------+
;
; void svm_vmrun(uint64_t vmcb_pa, struct GuestRegisters *regs)
;
; Loads guest GPRs from *regs, executes VMRUN, saves guest GPRs back to
; *regs, then returns. The VMCB state save area holds RSP, RIP, RFLAGS
; and RAX; we manage RBX-R15 + RDI + RSI manually.
;
; struct GuestRegisters layout (must match include/vmx.h):
;   rax=0, rbx=8, rcx=16, rdx=24, rbp=32, rsi=40, rdi=48,
;   r8=56, r9=64, r10=72, r11=80, r12=88, r13=96, r14=104, r15=112

global svm_vmrun

section .text

svm_vmrun:
    ; rdi = vmcb_pa   (physical address, used in RAX for VMRUN)
    ; rsi = *regs     (pointer to GuestRegisters)

    ; Save host callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Save both arguments on the stack
    push rdi    ; [rsp+0] = vmcb_pa
    push rsi    ; [rsp+0] = regs,  [rsp+8] = vmcb_pa

    ; Load guest GPRs from *regs (rsi still valid here)
    mov rbx, [rsi +   8]
    mov rcx, [rsi +  16]
    mov rdx, [rsi +  24]
    mov rbp, [rsi +  32]
    mov r8,  [rsi +  56]
    mov r9,  [rsi +  64]
    mov r10, [rsi +  72]
    mov r11, [rsi +  80]
    mov r12, [rsi +  88]
    mov r13, [rsi +  96]
    mov r14, [rsi + 104]
    mov r15, [rsi + 112]
    mov rdi, [rsi +  48]   ; guest rdi (before clobbering rsi)
    mov rsi, [rsi +  40]   ; guest rsi — overwrites pointer, do this last

    ; RAX = VMCB physical address (from stack, [rsp+8] after push rsi)
    mov rax, [rsp + 8]

    ; Enter guest — VMRUN saves host state to host save area,
    ; loads guest state from VMCB, executes guest until VMEXIT,
    ; then restores host state and returns here.
    vmrun

    ; === VM exit ===
    ; RSP is restored from host save area (same value as before VMRUN).
    ; All GPRs (including RAX) contain guest values at time of exit.
    ; Guest RAX is also saved in VMCB.state.rax by hardware.
    ; Stack: [rsp+0]=regs, [rsp+8]=vmcb_pa

    ; Temporarily stash registers we need
    push rax    ; [rsp+0]=guest_rax,  [rsp+8]=regs, [rsp+16]=vmcb_pa
    push rdi    ; [rsp+0]=guest_rdi,  [rsp+8]=guest_rax, [rsp+16]=regs

    ; Recover the regs pointer
    mov rdi, [rsp + 16]

    ; Save guest GPRs into *regs
    mov rax, [rsp + 8]      ; guest rax from stack
    mov [rdi +   0], rax
    mov [rdi +   8], rbx
    mov [rdi +  16], rcx
    mov [rdi +  24], rdx
    mov [rdi +  32], rbp
    mov [rdi +  40], rsi
    mov rax, [rsp]          ; guest rdi from stack
    mov [rdi +  48], rax
    mov [rdi +  56], r8
    mov [rdi +  64], r9
    mov [rdi +  72], r10
    mov [rdi +  80], r11
    mov [rdi +  88], r12
    mov [rdi +  96], r13
    mov [rdi + 104], r14
    mov [rdi + 112], r15

    ; Unwind stashed registers and saved args
    add rsp, 32     ; discard guest_rdi, guest_rax, regs, vmcb_pa

    ; Restore host callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
