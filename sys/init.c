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
#include <pc_console.h>
#include <debug_console.h>

struct ConsoleDisplay main_display;
struct CharacterDisplay *disp = (struct CharacterDisplay *)&main_display;

void __noreturn hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
#if 0
    const char hw[] = "HypoV Hypervisor - Copyright (C) Akos Kovacs";

    hv_console_display_init(&main_display);
    hv_disp_clear(disp);
    hv_set_stdout(disp);

    puts(hw);
#endif 

    hv_console_display_init(&main_display);
    hv_set_stdout(disp);
    dc_start(&main_display);

    while (1) 
        ;
}
