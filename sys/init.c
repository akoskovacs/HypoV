/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor initialization code                             |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <boot/multiboot.h>
#include <types.h>
#include <hypervisor.h>
#include <cpu.h>
#include <drivers/video/pc_console.h>
#include <drivers/input/pc_keyboard.h>
#include <debug_console.h>
#include <memory.h>
#include <loader.h>

static struct SystemInfo system_info;

void __noreturn hv_entry(struct MultiBootInfo *mbi)
{
    struct ConsoleDisplay main_display;

    system_info.s_boot_info = mbi;
    system_info.s_display   = (struct CharacterDisplay *)&main_display;

    keyboard_init();
    hv_console_display_init(&main_display);
    hv_set_stdout(system_info.s_display);
    system_info.s_phy_maps = mm_init_mapping(mbi);
    system_info.s_cpu_info = cpu_get_info();
    hv_console_cursor_disable();
    dc_start(&system_info);

    keyboard_loop(dc_keyboard_handler);

    while (1)
        ;
}
