/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | 32bit Debug Console                                        |
 * +------------------------------------------------------------+
*/
#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <drivers/video/pc_console.h>
#include <types.h>
#include <boot/multiboot.h>

struct DebugScreen;
struct PhysicalMMapping;

typedef int (*dc_draw_screen_ft)(struct DebugScreen *);
typedef int (*dc_handle_key_ft)(struct DebugScreen *, char key);

struct DebugScreen {
    int                  dc_key;
    const char          *dc_name;
    const char          *dc_help;
    dc_draw_screen_ft    dc_draw_handler;
    dc_handle_key_ft     dc_key_handler;
    #if 0
    /* Coordinates inside the display */
    int                  dc_x;
    int                  dc_y;
    int                  dc_width;
    int                  dc_height;
    #endif
};

int dc_start(struct ConsoleDisplay *, struct MultiBootInfo *mbi, struct PhysicalMMapping *mmap);
int dc_init(struct ConsoleDisplay *);
int dc_show_screen(struct DebugScreen *);
int dc_top_menu_draw(struct DebugScreen *);
int dc_bottom_menu_draw(struct DebugScreen *);
int dc_keyboard_handler(char scancode);
int dc_bottom_show_message(struct DebugScreen *scr, const char *message);

#endif // DEBUG_CONSOLE_H