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
    const char *int_msg = cpu_exceptions[14];
    int inum = frame->int_number;
    bochs_breakpoint();
    if (frame->int_number < sizeof(cpu_exceptions)/sizeof(char *)) {
        int_msg = cpu_exceptions[inum];
    }
#if 1
    hv_disp_puts_xy(display, 0, 0, int_msg);
    hv_disp_puts(&debug_serial, int_msg);
    hv_printf(&debug_serial, "Interrupt %s (%d), error code: %d\n"
        , cpu_exceptions[inum], inum, frame->error_code);
    hv_printf(display, "Interrupt %s (%d), error code: %d\n"
        , cpu_exceptions[inum], inum, frame->error_code);
#else
    volatile uint16_t *videoram = PC_VIDEORAM_BASE_ADDRESS;
    console_font_t font = BG_COLOR_CYAN | FG_COLOR_WHITE | LIGHT;
    i = 0;
    while (*int_msg) {
        videoram[(j * CONFIG_CONSOLE_WIDTH) + (i++)] = (console_font_t)((*int_msg++) | (font << 8));
    }
    if (j >= 24) {
        j = 0;
    } else {
        j++;
    }
#endif
}
