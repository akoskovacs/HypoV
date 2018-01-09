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
#if 0
    struct MemoryMap *hvmap = system_info.s_core_map;
    if (error != 0) {
        hv_printf(system_info.s_display, "error %d", error);
    } else if (hvmap != NULL) {
        hv_printf(system_info.s_display, "start: %X%X, end: %X%X\n", hvmap->mm_start, hvmap->mm_end);
        struct Elf64_Image *im = ld_load_hvcore(hvmap, &error);
        if (error == 0) {
            hv_printf(system_info.s_display, "ELF successfully loaded\n");
            /* WHOA, calling it */
            hv_printf(system_info.s_display, "Entry function: %x\n", im->i_entry);
            cpu_init_long_mode(&system_info);
            ld_call_hvcore(&system_info);
        } else {
            hv_printf(system_info.s_display, "error %d", error);
            hv_printf(system_info.s_display, "ELF load failed :(\n");
        }
        //dc_start(&system_info);
    } else {
        dc_start(&system_info);
    }
#else
    dc_start(&system_info);
#endif 

    keyboard_loop(dc_keyboard_handler);

    while (1)
        ;
}
