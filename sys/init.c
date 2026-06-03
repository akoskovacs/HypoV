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
#include <print.h>

static struct SystemInfo system_info;

static struct SystemInfo *system_info_init(struct SystemInfo *sysinfo)
{
    sysinfo->s_boot_info   = NULL;
    sysinfo->s_display     = NULL;
    sysinfo->s_cpu_info    = NULL;
    sysinfo->s_phy_maps    = NULL;
    sysinfo->s_core_image  = NULL;
    sysinfo->s_core_map    = NULL;
    sysinfo->s_pml4        = NULL;

    return sysinfo;
}

void __noreturn hv_entry(struct MultiBootInfo *mbi)
{
    int error = 0;
    struct ConsoleDisplay main_display;

    cpu_tables_init();
    system_info_init(&system_info);
    system_info.s_boot_info = mbi;
    system_info.s_display   = (struct CharacterDisplay *)&main_display;

    keyboard_init();
    hv_console_display_init(&main_display);
    hv_set_stdout(system_info.s_display);
    system_info.s_phy_maps = mm_init_mapping(mbi);
    system_info.s_cpu_info = cpu_get_info();
    system_info.s_core_map = mm_alloc_phymap(system_info.s_phy_maps, 16, &error);
    hv_console_cursor_disable();

    dc_start(&system_info);
    keyboard_loop(dc_keyboard_handler);

    while (1)
        ;
}
