; +---------------------------------------------------------------------+
; | HypoV guest boot stub (16-bit real mode, ORG 0x8000)               |
; |                                                                     |
; | Reads the MBR from the first HDD (drive 0x80) to 0000:7C00 using   |
; | BIOS INT 13h, verifies the 0x55AA signature, and jumps to it.      |
; |                                                                     |
; | If no HDD or invalid MBR: injects Down+Enter into the BIOS         |
; | keyboard buffer so the guest GRUB auto-selects entry 1             |
; | (the proof kernel) instead of looping back to HypoV, then         |
; | calls INT 19h to bootstrap from the proof CD.                      |
; +---------------------------------------------------------------------+

[BITS 16]
[ORG 0x8000]

boot_stub_start:
    mov  dx, 0x3F8
    mov  al, 'B'
    out  dx, al

    ; Set up read destination: ES:BX = 0000:7C00
    xor  ax, ax
    mov  es, ax
    mov  bx, 0x7C00

    ; INT 13h AH=02: Read Sectors from Drive
    mov  ah, 0x02            ; function: read
    mov  al, 0x01            ; sector count: 1
    mov  ch, 0x00            ; cylinder 0
    mov  cl, 0x01            ; sector 1 (1-based)
    mov  dh, 0x00            ; head 0
    mov  dl, 0x80            ; drive 0x80 = first HDD
    int  0x13
    jc   .fallback           ; CF=1: read error — no HDD or I/O failure

    ; Verify the standard MBR boot signature (bytes 510-511 = 55 AA)
    cmp  word [es:0x7DFE], 0xAA55
    jne  .fallback           ; signature missing — not a bootable MBR

    jmp  0x0000:0x7C00       ; execute loaded MBR

.fallback:
    ; No bootable HDD — stuff Down+Enter into BIOS keyboard buffer so
    ; the guest GRUB auto-selects entry 1 (hyp_check proof kernel).
    mov  ah, 0x05
    mov  cx, 0x5000          ; Down arrow: scan=0x50, ascii=0x00
    int  0x16
    mov  ah, 0x05
    mov  cx, 0x1C0D          ; Enter: scan=0x1C, ascii=0x0D
    int  0x16
    ; Bootstrap from the next device (the proof CD-ROM)
    int  0x19
    ; INT 19h returned — nothing else to try
    hlt
    jmp  $
