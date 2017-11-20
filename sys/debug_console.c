/*
 * +---------------------------------------------------------------------+
 * | Copyright (C) Kovács Ákos - 2017                                    |
 * |                                                                     |
 * | Debug Console implementation (in 32bit protected mode)              |
 * +---------------------------------------------------------------------+
*/

#include <drivers/input/pc_keyboard.h>
#include <debug_console.h>
#include <error.h>
#include <system.h>

enum DC_KEYS {
    DS_F1 = 1,
    DS_F2,
    DS_F3,
    DS_F4,
    DS_F5
};

#define DECL_HANDLER_PROTOTYPES(scrname)                             \
    static int dc_##scrname##_info_show(struct DebugScreen *);       \
    static int dc_##scrname##_handle_key(struct DebugScreen *, char) \

DECL_HANDLER_PROTOTYPES(vmm);
DECL_HANDLER_PROTOTYPES(cpu);
DECL_HANDLER_PROTOTYPES(mem);
DECL_HANDLER_PROTOTYPES(disk);
DECL_HANDLER_PROTOTYPES(guest);

static struct ConsoleDisplay *current_display = NULL;
static struct DebugScreen *current_screen = NULL;
static struct DebugScreen ds_screen[] = {
    { DS_F1, "HYPERVISOR", "Hypervisor info, settings and functions", dc_vmm_info_show, dc_vmm_handle_key },
    { DS_F2, "CPU", "Physical CPU information", dc_cpu_info_show, dc_cpu_handle_key },
    { DS_F3, "MEMORY", "System memory-mapping information", dc_mem_info_show, dc_mem_handle_key },
    { DS_F4, "DISK", "Boot partition listings", dc_disk_info_show, dc_disk_handle_key },
    { DS_F5, "VM", "Guest virtualization OS management", dc_guest_info_show, dc_guest_handle_key },
};
#define NR_SCREENS (sizeof(ds_screen)/sizeof(struct DebugScreen))

int dc_start(struct ConsoleDisplay *disp)
{
    if (disp == NULL) {
        return -HV_ENODISP;
    }
    current_display = disp;
    /* Display the first screen */
    return dc_show_screen(ds_screen);
}

int dc_show_screen(struct DebugScreen *scr)
{
    if (scr == NULL) {
        return -HV_ENODISP;
    }
    current_screen = scr;
    hv_console_clear(current_display);
    dc_top_menu_draw(scr);
    dc_bottom_menu_draw(scr);
    return scr->dc_draw_handler(scr);
}

int dc_top_menu_draw(struct DebugScreen *scr)
{
    const char branding[] = " HypoV ";
    char sw_tmpl[] = " F1 - ";
    struct DebugScreen *scrs = ds_screen;
    int i;
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;

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
        hv_disp_puts(cdisp, sw_tmpl);
        hv_disp_puts(cdisp, ds_screen[i].dc_name);
        hv_disp_putc(cdisp, ' ');
    }
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
    hv_console_fill_line(current_display, current_display->pv_x, 0, CONSOLE_WIDTH(current_display)-current_display->pv_x);
    hv_console_set_xy(current_display, CONSOLE_WIDTH(current_display)-sizeof(branding), 0);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN | LIGHT);
    hv_disp_puts(cdisp, branding);
    return 0;
}

int dc_bottom_menu_draw(struct DebugScreen *scr)
{
    int i;
    const char copyright[] = "Copyright (C) Akos Kovacs ";
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    int x = CONSOLE_WIDTH(current_display) - sizeof(copyright);

    hv_console_set_xy(current_display, 1, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN | LIGHT);
    hv_console_fill_line(current_display, 0, CONSOLE_LAST_ROW(current_display), CONSOLE_WIDTH(current_display));
    hv_disp_puts(cdisp, scr->dc_help);
    hv_console_set_xy(current_display, x, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
    hv_disp_puts(cdisp, copyright);
    return 0;
}

int dc_keyboard_handler(char scancode)
{
    int i;
    switch (scancode) {
        case KEY_F1:
        dc_show_screen(ds_screen);
        break;

        case KEY_F2:
        dc_show_screen(ds_screen+1);
        break;

        case KEY_F3:
        dc_show_screen(ds_screen+2);
        break;

        case KEY_F4:
        dc_show_screen(ds_screen+3);
        break;

        case KEY_F5:
        dc_show_screen(ds_screen+4);
        break;

        case KEY_TAB:
        if (current_screen) {
            i = current_screen->dc_key % NR_SCREENS;
            dc_show_screen(ds_screen+i);
            /* TODO: Delete this delay loop */
            for (i = 1; i < 1e7; i++)
                ;
        }
        return 0; 
    }

    if (current_screen) {
        current_screen->dc_key_handler(current_screen, scancode);
    }
    return 0;
}

static int dc_vmm_info_show(struct DebugScreen *scr)
{
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED);
    hv_console_set_xy(current_display, 10, 5);
    hv_disp_puts(cdisp, "Version");
    hv_console_set_xy(current_display, 30, 5);
    hv_disp_puts(cdisp, "0.1-prealpha");

    hv_console_set_xy(current_display, 10, 6);
    hv_disp_puts(cdisp, "Bit width");
    hv_console_set_xy(current_display, 30, 6);
    hv_disp_puts(cdisp, "32 bit");

    hv_console_set_xy(current_display, 10, 7);
    hv_disp_puts(cdisp, "Built at");
    hv_console_set_xy(current_display, 30, 7);
    hv_disp_puts(cdisp, __DATE__ ", " __TIME__);

    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(current_display, 10, 20);
    hv_disp_puts(cdisp, "[R] - Press R to restart the computer");
    
}

static int dc_vmm_handle_key(struct DebugScreen *scr, char key)
{
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    if (key == KEY_R) {
        hv_console_set_xy(current_display, 10, 21);
        hv_disp_puts(cdisp, "Restarting...");
        sys_reboot();
    }
    return 0;
}

static char vendor[13];
static char branding[49];
static char *branding_start = NULL;

static int dc_cpu_info_show(struct DebugScreen *scr)
{
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED);
    hv_console_set_xy(current_display, 10, 5);
    hv_disp_puts((struct CharacterDisplay *)current_display, "Branding");

    hv_console_set_xy(current_display, 30, 5);
    if (branding_start == NULL) {
        cpuid_get_branding(branding);
        branding_start = branding;
        while (*branding_start == ' ') {
            branding_start++;
        }

        cpuid_get_vendor(vendor);
    }

    hv_disp_puts((struct CharacterDisplay *)current_display, branding_start);

    hv_console_set_xy(current_display, 10, 6);
    hv_disp_puts((struct CharacterDisplay *)current_display, "Vendor string");
    hv_console_set_xy(current_display, 30, 6);
    hv_disp_puts((struct CharacterDisplay *)current_display, vendor);

    return -HV_ENOIMPL;
}

static int dc_cpu_handle_key(struct DebugScreen *scr, char key)
{
    return -HV_ENOIMPL;
}

static int dc_mem_info_show(struct DebugScreen *scr)
{
    hv_console_set_xy(current_display, 26, 11);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_disp_puts((struct CharacterDisplay *)current_display, "< Implementation required >");
    return -HV_ENOIMPL;
}
static int dc_mem_handle_key(struct DebugScreen *scr, char key) { return -HV_ENOIMPL; }

static int dc_disk_info_show(struct DebugScreen *scr) { return dc_mem_info_show(scr); }
static int dc_disk_handle_key(struct DebugScreen *scr, char key) { return -HV_ENOIMPL; }

static int dc_guest_info_show(struct DebugScreen *scr) { return dc_mem_info_show(scr); }
static int dc_guest_handle_key(struct DebugScreen *scr, char key) { return -HV_ENOIMPL; }