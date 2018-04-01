/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Global interrupt handler and setup code                    |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <drivers/video/pc_console.h>
#include <system.h>
#include <cpu.h>

inline void idt64_make_entry(struct IDT64Entry *ent, bool is_trap, uint16_t seg, hv_int_handler_ft handler)
{
    unsigned long intr_addr = (unsigned long)handler;
    ent->offset_0_15  = (uint16_t)intr_addr & 0xFFFF;
    ent->offset_16_31 = (uint16_t)(intr_addr >> 16) & 0xFFFF;
    ent->offset_32_63 = (uint32_t)(intr_addr >> 32) & 0xFFFFFFFF;
    ent->segment_sel  = seg;
    ent->flags        = IDT_PRESENT | (is_trap ? IDT_TYPE_TRAP : IDT_TYPE_INT);
    ent->reserved0    = 0;
}

void int_handler(void)
{
    static int i = 0;
    static int j = 0;
    console_font_t font = BG_COLOR_CYAN | FG_COLOR_WHITE | LIGHT;
    volatile console_font_t *videoram = PC_VIDEORAM_BASE_ADDRESS;
    char msg[] = "Got an interrupt!!!";
    char *int_msg = msg;
    while (*int_msg) {
        videoram[(j * CONFIG_CONSOLE_WIDTH) + (i++)] = *int_msg++ | (font << 8);
    }
    j++;
}
