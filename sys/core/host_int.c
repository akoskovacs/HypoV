/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Global interrupt handler and setup code                    |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <drivers/video/pc_console.h>
#include <char_display.h>
#include <system.h>
#include <cpu.h>
#include <interrupt.h>
#include <print.h>

extern struct ConsoleDisplay main_display;
extern struct CharacterDisplay *display;
extern struct CharacterDisplay debug_serial;

void idt64_make_entry(struct IDT64Entry *ent, bool is_trap, uint16_t seg, hv_int_handler_ft handler)
{
    unsigned long intr_addr = (unsigned long)handler;
    ent->offset_0_15  = (uint16_t)intr_addr & 0xFFFF;
    ent->offset_16_31 = (uint16_t)(intr_addr >> 16) & 0xFFFF;
    ent->offset_32_63 = (uint32_t)(intr_addr >> 32) & 0xFFFFFFFF;
    ent->segment_sel  = seg;
    ent->flags        = IDT_PRESENT | (is_trap ? IDT_TYPE_TRAP : IDT_TYPE_INT);
    ent->reserved0    = 0;
}

static char *cpu_exceptions[] = {
    "Divide by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Brakepoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun???",
    "Invalid TSS",
    "Segment Not Present",
    "General Protection Fault",
    "Page Fault",
    "[Reserved]",
    "Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "[Reserved]",
    "Triple Fault",
    "[Reserved]"
};
volatile static int i, j = 0;

void hv_handle_interrupt(struct TrapFrame *frame)
{
    int inum = frame->int_number;

    /* Hardware IRQs (PIC remapped to 0x20-0x2F) */
    if (inum >= 0x20 && inum < 0x30) {
        pic_send_eoi(inum - 0x20);
        return;
    }

    /* CPU exceptions */
    const char *int_msg = "Unknown";
    if (inum < (int)(sizeof(cpu_exceptions)/sizeof(char *))) {
        int_msg = cpu_exceptions[inum];
    }

    hv_printf(&debug_serial, "Exception: %s (%d), error code: %d\n",
        int_msg, inum, frame->error_code);
    hv_printf(display, "Exception: %s (%d), error: %d\n",
        int_msg, inum, frame->error_code);
}
