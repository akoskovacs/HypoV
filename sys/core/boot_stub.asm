; +---------------------------------------------------------------------+
; | HypoV guest boot stub (16-bit real mode, ORG 0x8000)               |
; |                                                                     |
; | Reads the MBR from the first HDD (drive 0x80) to 0000:7C00 using   |
; | BIOS INT 13h, verifies the 0x55AA signature, and jumps to it.      |
; | Falls back to INT 19h (BIOS bootstrap) if the HDD is missing or    |
; | the sector is not a valid MBR — allowing the BIOS to boot from the |
; | next available device (e.g. a proof CD-ROM).                       |
; +---------------------------------------------------------------------+

[BITS 16]
[ORG 0x8000]

boot_stub_start:
    xor  ax, ax
    mov  es, ax              ; ES = 0
    mov  bx, 0x7C00          ; ES:BX = 0000:7C00 (read destination)

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
    ; No valid HDD MBR — ask the BIOS to try the next boot device.
    ; On a proof setup the BIOS finds the proof CD and boots GRUB.
    int  0x19
    cli                      ; INT 19h returned — nothing to boot
    hlt
    jmp  $
