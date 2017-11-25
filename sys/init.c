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
#include <drivers/video/pc_console.h>
#include <drivers/input/pc_keyboard.h>
#include <debug_console.h>
#include <memory.h>

/* If the magic number is wrong, mbi will be NULL */
void __noreturn hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
    struct ConsoleDisplay main_display;
    struct CharacterDisplay *disp = (struct CharacterDisplay *)&main_display;
    struct PhysicalMMapping *system_mmap = NULL;

    keyboard_init();
    hv_console_display_init(&main_display);
    hv_set_stdout(disp);
    system_mmap = hv_mm_init_mapping(mbi);
    hv_console_cursor_disable();
    dc_start(&main_display, mbi, system_mmap);

    keyboard_loop(dc_keyboard_handler);

    while (1)
        ;
}
