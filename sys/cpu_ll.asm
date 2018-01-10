; +------------------------------------------------------------+
; | Copyright (C) Ákos Kovács - 2018                           |
; |                                                            |
; | Low-level CPU stuff                                        |
; +------------------------------------------------------------+

global __cpu_long_mode_enter
global __cpu_call_64

; %include "cpu_ll.inc"

%define CR0_PG_BIT 31

%define DESC_32BIT          (1 << 22)
%define DESC_64BIT          (1 << 21)
%define DESC_PRESENT        (1 << 15)
%define DESC_CS_READ        (1 << 9)
%define DESC_DS_WRITE       (1 << 9)
%define DESC_DATA           (1 << 12); bit 11 = 0
%define DESC_CODE           (1 << 12)|(1 << 11)
%define DESC_TSS            (1 << 8)|(1 << 11)
%define DESC_SEGLIMIT_HIGH  (0xF << 16)

align 4
section .text
bits 32
__cpu_long_mode_enter:
;   xchg bx, bx         ; Bochs breakpoint, if needed
    mov eax, cr0        ; Read CR0
    bts eax, CR0_PG_BIT ; Set PG bit
    mov cr0, eax
    jmp .arch64         ; Should be in 64bit
.arch64:
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
; void __noreturn __cpu_call_64(uint32_t jmp_addr, uint32_t arg0);
__cpu_call_64:
    xchg bx, bx
    ; This function won't return, no need for 
    ; subroutine prologues, just get the first two
    ; parameters (uint32_t jmp_addr, uint32_t arg0)
    mov eax, [esp + 4]
    ; The second parameter uses the 64bit 
    ; calling convention. The function parameter 
    ; will appear in the RDI register.
    mov edi, [esp + 8]
    ; Modify target on-the-fly
    mov dword [.jmp_addr], eax
    ; Setup an empty IDTR
    lidt [idt_segment_64]
    ; Setup GDTR
    lgdt [gdt_segment_64]
    ; Use the new 64 bit segments
    mov eax, 0x20
    mov ss, ax
    mov ds, eax
    mov es, eax
    mov fs, eax
    mov gs, eax
    ; Self-modifying machine code to jump directly to the
    ; desired 64bit code.
    db 0xEA ; jmp far cs:whatever
.jmp_addr:
    dd 0x0  ; Address from EAX
    dw 0x18 ; 64 bit CS selector
ret ; __noreturn
; End of __cpu_call_64

section .data
%define GDT_NR_ENTRIES 5
; Unfortunately, a valid 64bit GDT is still needed.
; It is easier to set that up here, in compatilibty mode.
align 8
gdt_64:
; Null entry [0x0]
    dd 0x0 ; limit, base address
    dd 0x0 ; limit, base address and flags
; 32bit CS entry [0x8]
    dw 0xFFFF ; max segment limit
    dw 0x0    ; base address
    dd DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_32BIT | DESC_SEGLIMIT_HIGH
; 32bit SS, DS, ES, FS, GS [0x10]
    dw 0xFFFF ; max segment limit
    dw 0x0    ; base address
    dd DESC_PRESENT | DESC_DATA | DESC_32BIT | DESC_DS_WRITE | DESC_SEGLIMIT_HIGH
; 64bit CS entry [0x18]
    dw 0xFFFF ; max segment limit
    dw 0x0    ; base address
    dd DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_64BIT | DESC_SEGLIMIT_HIGH
; 64bit SS, DS, ES, FS, GS [0x20]
    dw 0xFFFF ; max segment limit
    dw 0x0    ; base address
    dd DESC_PRESENT | DESC_DATA | DESC_DS_WRITE | DESC_64BIT | DESC_SEGLIMIT_HIGH
; 64 bit Task State Segment
    ; TSS TODO
; 64 bit Interrupt Descriptor
    ; IDT TODO

align 8
gdt_segment_64:
    dw 8 * GDT_NR_ENTRIES
    dd gdt_64
    dd 0x0

; Dummy LDT
align 8
ldt_segment_64:
    dw 0x0
    dq 0x0

; Dummy IDT
align 8
idt_segment_64:
    dw 0x0
    dq 0x0