; +---------------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2024                                    |
; |                                                                     |
; | Low-level VMX instruction wrappers and VM exit trampoline           |
; +---------------------------------------------------------------------+

global vmx_on
global vmx_off
global vmx_clear
global vmx_ptr_load
global vmx_read
global vmx_write
global vmx_launch
global vmx_resume
global vmx_exit_trampoline

extern vmx_exit_handler

section .text

; int vmx_on(uint64_t *vmxon_pa)
;   rdi = pointer to 64-bit physical address of VMXON region
;   returns 0 on success, -1 on failure (CF or ZF set)
vmx_on:
    vmxon QWORD [rdi]
    jc    .fail
    jz    .fail
    xor   eax, eax
    ret
.fail:
    mov   eax, -1
    ret

; void vmx_off(void)
vmx_off:
    vmxoff
    ret

; int vmx_clear(uint64_t *vmcs_pa)
;   rdi = pointer to 64-bit physical address of VMCS region
vmx_clear:
    vmclear QWORD [rdi]
    jc    .fail
    jz    .fail
    xor   eax, eax
    ret
.fail:
    mov   eax, -1
    ret

; int vmx_ptr_load(uint64_t *vmcs_pa)
;   rdi = pointer to 64-bit physical address of VMCS region
vmx_ptr_load:
    vmptrld QWORD [rdi]
    jc    .fail
    jz    .fail
    xor   eax, eax
    ret
.fail:
    mov   eax, -1
    ret

; uint64_t vmx_read(uint32_t field)
;   rdi = VMCS field encoding
;   returns field value in rax
vmx_read:
    xor   rax, rax
    vmread rax, rdi
    ret

; void vmx_write(uint32_t field, uint64_t value)
;   rdi = VMCS field encoding
;   rsi = value to write
vmx_write:
    vmwrite rdi, rsi
    ret

; int vmx_launch(void)
;   Performs the first VM entry. Does not return on success (guest runs).
;   Returns -1 if VMLAUNCH fails (CF or ZF set).
vmx_launch:
    vmlaunch
    mov   eax, -1
    ret

; int vmx_resume(void)
;   Re-enters the guest after a VM exit. Does not return on success.
;   Returns -1 if VMRESUME fails.
vmx_resume:
    vmresume
    mov   eax, -1
    ret

; vmx_exit_trampoline
;   This is HOST_RIP — the CPU jumps here on every VM exit.
;   At entry: RSP = HOST_RSP (from VMCS), all GPRs hold guest values.
;   We save guest GPRs, call the C handler, restore them, then VMRESUME.
;
;   The pushed register layout matches struct GuestRegs:
;     { rax, rbx, rcx, rdx, rbp, rsi, rdi, r8..r15 }
vmx_exit_trampoline:
    ; Save all guest GPRs (RSP was restored from VMCS HOST_RSP)
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

    ; Call C handler with pointer to saved register frame
    mov  rdi, rsp
    call vmx_exit_handler

    ; Restore guest GPRs (handler may have modified them for CPUID, VMCALL, etc.)
    pop  rax
    pop  rbx
    pop  rcx
    pop  rdx
    pop  rbp
    pop  rsi
    pop  rdi
    pop  r8
    pop  r9
    pop  r10
    pop  r11
    pop  r12
    pop  r13
    pop  r14
    pop  r15

    vmresume
    ; VMRESUME failed — unrecoverable
    ud2

section .note.GNU-stack noalloc noexec nowrite progbits
