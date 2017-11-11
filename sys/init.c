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

struct ConsoleDisplay main_display;
struct CharacterDisplay *disp = (struct CharacterDisplay *)&main_display;

void __noreturn hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
    const char hw[] = "HypoV Hypervisor - Copyright (C) Akos Kovacs";

    hv_console_display_init(&main_display);
    hv_disp_clear(disp);
    hv_set_stdout(disp);

    puts(hw);

    while (1) 
        ;
}
