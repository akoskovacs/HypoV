/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | 32bit Debug Console                                        |
 * +------------------------------------------------------------+
*/
#ifndef DEBUG_CONSOLE_H
#define DEBUG_CONSOLE_H

#include <pc_console.h>

struct DebugScreen;

typedef int (*dc_draw_screen_ft)(struct DebugScreen *);
typedef int (*dc_handle_key_ft)(struct DebugScreen *, int key);

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

int dc_start(struct ConsoleDisplay *);
int dc_init(struct ConsoleDisplay *);
int dc_show_screen(struct DebugScreen *);
int dc_top_menu_draw(struct DebugScreen *);
int dc_bottom_menu_draw(struct DebugScreen *);

#endif // DEBUG_CONSOLE_H