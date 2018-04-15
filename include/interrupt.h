/*
 * +---------------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                                    |
 * |                                                                     |
 * | Interrupt handling stuff                                            |
 * +---------------------------------------------------------------------+
*/

#include <basic.h>
#include <types.h>

#define NR_HV_INTERRUPTS 256
#define IDT_PRESENT      (1 << 15)
#define IDT_DPL_SHIFT    13
#define IDT_DPL_MASK     0x6000
#define IDT_TYPE_INT     0xE00
#define IDT_TYPE_TRAP    0xF00

/* Interrupt/trap gate descriptor entries */
struct __packed IDT32Entry {
    uint16_t ie_offset_0_15;    // Offset bits [0..15]
    uint16_t ie_selector;
    uint8_t  ie_reserved;       // Always zero
    uint8_t  ie_type_attribute;
    uint16_t ie_offset_16_31;   // Offset bits [16..31]
};

struct __packed IDT64Entry 
{
    uint16_t offset_0_15;
    uint16_t segment_sel;
    uint16_t flags;
    uint16_t offset_16_31;
    uint32_t offset_32_63;
    uint32_t reserved0;
};

/* Stack frame of the trap/interrupt handlers */
struct __packed TrapFrame {
    long rax;
    long rbx;
    long rcx;
    long rdx;
    long rbp;
    long rsi;
    long rdi;
    long r8;
    long r9;
    long r10;
    long r11;
    long r12;
    long r13;
    long r14;
    long r15;
    long int_number;
    long error_code;
    // long ret_addr;
};

/* Pointer to the actual handler */
typedef void (*hv_int_handler_ft)(void);

void idt64_make_entry(struct IDT64Entry *ent, bool is_trap, uint16_t seg, hv_int_handler_ft handler);
void hv_handle_interrupt(struct TrapFrame *frame);

int pic_init(int offset1, int offset2);
void pic_send_eoi(uint8_t irq);
void pic_disable(void);
