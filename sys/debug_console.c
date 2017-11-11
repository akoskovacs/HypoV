/*
 * +---------------------------------------------------------------------+
 * | Copyright (C) Kovács Ákos - 2017                                    |
 * |                                                                     |
 * | Debug Console implementation (in 32bit protected mode)              |
 * +---------------------------------------------------------------------+
*/

#include <debug_console.h>
#include <error.h>

enum DC_KEYS {
    DS_F1 = 1,
    DS_F2,
    DS_F3,
    DS_F4,
    DS_F5
};

#define DECL_HANDLER_PROTOTYPES(scrname)                             \
    static int dc_##scrname##_info_show(struct DebugScreen *);       \
    static int dc_##scrname##_handle_key(struct DebugScreen *, int)  \

DECL_HANDLER_PROTOTYPES(cpu);
DECL_HANDLER_PROTOTYPES(mem);
DECL_HANDLER_PROTOTYPES(disk);
DECL_HANDLER_PROTOTYPES(guest);

static struct ConsoleDisplay *current_display = NULL;
static struct DebugScreen ds_screen[] = {
    { DS_F1, "CPU", "Physical CPU information", dc_cpu_info_show, dc_cpu_handle_key },
    { DS_F2, "MEMORY", "System memory-mapping information", dc_mem_info_show, dc_mem_handle_key },
    { DS_F3, "DISK", "Boot partition listings", dc_disk_info_show, dc_disk_handle_key },
    { DS_F4, "VM", "Guest virtualization OS management", dc_cpu_info_show, dc_cpu_handle_key },
};
#define NR_SCREENS (sizeof(ds_screen)/sizeof(struct DebugScreen))

int dc_start(struct ConsoleDisplay *disp)
{
    if (disp == NULL) {
        return -HV_ENODISP;
    }
    current_display = disp;
    hv_console_clear(disp);
    /* Display the first screen */
    return dc_show_screen(ds_screen);
}

int dc_show_screen(struct DebugScreen *scr)
{
    if (scr == NULL) {
        return -HV_ENODISP;
    }
    dc_top_menu_draw(scr);
    dc_bottom_menu_draw(scr);
    return scr->dc_draw_handler(scr);
}

int dc_top_menu_draw(struct DebugScreen *scr)
{
    char sw_tmpl[] = " F1 - ";
    struct DebugScreen *scrs = ds_screen;
    int i;

    hv_console_set_attribute(current_display, FG_COLOR_BROWN | BG_COLOR_BLACK | LIGHT);
    hv_console_fill_line(current_display, 0, 0, CONSOLE_WIDTH(current_display));
    hv_console_set_xy(current_display, 0, 0);

    for (i = 0; i < NR_SCREENS; i++)  {
        sw_tmpl[2] = (char)('0' + ds_screen[i].dc_key);
        /* Highlight the current screen */
        if (scr == ds_screen+i) {
            hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
        } else {
            hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
        }
        hv_disp_puts((struct CharacterDisplay *)current_display, sw_tmpl);
        hv_disp_puts((struct CharacterDisplay *)current_display, ds_screen[i].dc_name);
        hv_disp_putc((struct CharacterDisplay *)current_display, ' ');
    }
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
    hv_console_fill_line(current_display, current_display->pv_x, 0, CONSOLE_WIDTH(current_display)-current_display->pv_x);
    return 0;
}

int dc_bottom_menu_draw(struct DebugScreen *scr)
{
    int i;
    const char copyright[] = "HypoV - Copyright (C) Akos Kovacs ";
    int x = CONSOLE_WIDTH(current_display) - sizeof(copyright);

    hv_console_set_xy(current_display, 1, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN | LIGHT);
    hv_console_fill_line(current_display, 0, CONSOLE_LAST_ROW(current_display), CONSOLE_WIDTH(current_display));
    hv_disp_puts((struct CharacterDisplay *)current_display, scr->dc_help);
    hv_console_set_xy(current_display, x, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
    hv_disp_puts((struct CharacterDisplay *)current_display, copyright);
    return 0;
}

static int dc_cpu_info_show(struct DebugScreen *scr)
{
    hv_console_set_xy(current_display, 26, 11);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_disp_puts((struct CharacterDisplay *)current_display, "< Implementation required >");
    return -HV_ENOIMPL;
}

static int dc_cpu_handle_key(struct DebugScreen *scr, int key)
{
    return -HV_ENOIMPL;
}

static int dc_mem_info_show(struct DebugScreen *scr) { return dc_cpu_info_show(scr); }
static int dc_mem_handle_key(struct DebugScreen *scr, int key) { return -HV_ENOIMPL; }

static int dc_disk_info_show(struct DebugScreen *scr) { return dc_cpu_info_show(scr); }
static int dc_disk_handle_key(struct DebugScreen *scr, int key) { return -HV_ENOIMPL; }

static int dc_guest_info_show(struct DebugScreen *scr) { return dc_cpu_info_show(scr); }
static int dc_guest_handle_key(struct DebugScreen *scr, int key) { return -HV_ENOIMPL; }