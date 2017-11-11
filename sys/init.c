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

volatile uint16_t *videomem = (uint16_t *)0xb8000;
#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

struct ConsoleDisplay main_display;

void __noreturn hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
    int i;
    const char hw[] = "HypoV Hypervisor - Copyright (C) Akos Kovacs";

    hv_console_display_init(&main_display);
    hv_disp_clear((struct CharacterDisplay *)&main_display);
    hv_set_stdout(&main_display);

    puts(hw);

    while (1) 
        ;
}
