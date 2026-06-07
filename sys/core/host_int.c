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

/* Captured by hv_handle_interrupt; consumed by the SVM INTR exit handler */
volatile uint8_t hv_pending_irq_vector = 0;

void hv_handle_interrupt(struct TrapFrame *frame)
{
    int inum = frame->int_number;

    /* Hardware IRQs (PIC remapped to 0x20-0x2F) */
    if (inum >= 0x20 && inum < 0x30) {
        hv_pending_irq_vector = (uint8_t)inum;
        pic_send_eoi(inum - 0x20);
        return;
    }

    /* CPU exceptions — use switch to avoid relocation issues with pointer arrays */
    const char *int_msg;
    switch (inum) {
    case 0:  int_msg = "Divide by Zero";           break;
    case 1:  int_msg = "Debug";                    break;
    case 2:  int_msg = "NMI";                      break;
    case 3:  int_msg = "Breakpoint";               break;
    case 4:  int_msg = "Overflow";                 break;
    case 5:  int_msg = "Bound Range";              break;
    case 6:  int_msg = "Invalid Opcode (#UD)";     break;
    case 7:  int_msg = "Device Not Available";     break;
    case 8:  int_msg = "Double Fault";             break;
    case 10: int_msg = "Invalid TSS";              break;
    case 11: int_msg = "Segment Not Present";      break;
    case 12: int_msg = "Stack Fault";              break;
    case 13: int_msg = "General Protection";       break;
    case 14: int_msg = "Page Fault";               break;
    default: int_msg = "Unknown";                  break;
    }

    /* RIP is on the stack just past the TrapFrame */
    unsigned long rip = *((unsigned long *)(frame + 1));

    hv_printf(&debug_serial, "EXCEPTION %d (%s) rip=%x:%x err=%d\n",
        inum, int_msg, (unsigned)(rip >> 32), (unsigned)rip, (int)frame->error_code);
    hv_printf(display, "EXCEPTION %d (%s) err=%ld\n",
        inum, int_msg, frame->error_code);

    /* CPU exceptions are fatal — halt to avoid infinite re-entry loops */
    while (1)
        __asm__ __volatile__("hlt");
}
