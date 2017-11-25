/*
 * +---------------------------------------------------------------------+
 * | Copyright (C) Kovács Ákos - 2017                                    |
 * |                                                                     |
 * | Debug Console implementation (in 32bit protected mode)              |
 * +---------------------------------------------------------------------+
*/

#include <types.h>
#include <drivers/input/pc_keyboard.h>
#include <debug_console.h>
#include <print.h>
#include <error.h>
#include <system.h>
#include <memory.h>

#ifndef CONFIG_NR_MMAP_MAX_ENTRIES
# define CONFIG_NR_MMAP_MAX_ENTRIES 10
#endif

enum DC_KEYS {
    DS_F1 = 1,
    DS_F2,
    DS_F3,
    DS_F4,
    DS_F5
};

/* TODO: Delete this delay loop */
#define KBD_DELAY() for (int i = 1; i < 5e7; i++)

#define DECL_HANDLER_PROTOTYPES(scrname)                             \
    static int dc_##scrname##_info_show(struct DebugScreen *);       \
    static int dc_##scrname##_handle_key(struct DebugScreen *, char) \

DECL_HANDLER_PROTOTYPES(vmm);
DECL_HANDLER_PROTOTYPES(cpu);
DECL_HANDLER_PROTOTYPES(mem);
DECL_HANDLER_PROTOTYPES(disk);
DECL_HANDLER_PROTOTYPES(guest);

static struct ConsoleDisplay *current_display    = NULL;
static struct DebugScreen *current_screen        = NULL;
static struct MultiBootInfo *boot_info           = NULL;
static struct PhysicalMMapping *sys_mmap         = NULL;

static struct DebugScreen ds_screen[] = {
    { DS_F1, "HYPERVISOR", "Hypervisor info, settings and functions", dc_vmm_info_show, dc_vmm_handle_key },
    { DS_F2, "CPU", "Physical CPU information", dc_cpu_info_show, dc_cpu_handle_key },
    { DS_F3, "MEMORY", "System memory-mapping information", dc_mem_info_show, dc_mem_handle_key },
    { DS_F4, "DISK", "Boot partition listings", dc_disk_info_show, dc_disk_handle_key },
    { DS_F5, "VM", "Guest virtualization OS management", dc_guest_info_show, dc_guest_handle_key },
};
#define NR_SCREENS (sizeof(ds_screen)/sizeof(struct DebugScreen))

int dc_start(struct ConsoleDisplay *disp, struct MultiBootInfo *mbi, struct PhysicalMMapping *mmap)
{
    if (disp == NULL) {
        return -HV_ENODISP;
    }
    current_display = disp;
    boot_info       = mbi;
    sys_mmap        = mmap;
    /* Display the first screen */
    return dc_show_screen(ds_screen);
}

int dc_show_screen(struct DebugScreen *scr)
{
    if (scr == NULL) {
        return -HV_ENODISP;
    }
    current_screen = scr;
    hv_console_clear((struct CharacterDisplay *)current_display);
    dc_top_menu_draw(scr);
    dc_bottom_menu_draw(scr);
    return scr->dc_draw_handler(scr);
}

int dc_top_menu_draw(struct DebugScreen *scr)
{
    char sw_tmpl[] = " F1 - ";
    const char branding[] = " HypoV ";
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
    hv_console_fill_line(current_display, current_display->pv_x, 0
            , CONSOLE_WIDTH(current_display)-(current_display->pv_x));
    hv_console_set_xy(current_display, CONSOLE_WIDTH(current_display)-sizeof(branding), 0);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN | LIGHT);
    hv_disp_puts(cdisp, branding);
    return 0;
}

int dc_bottom_show_message(struct DebugScreen *scr, const char *message)
{
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;

    hv_console_set_xy(current_display, 1, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN | LIGHT);
    hv_console_fill_line(current_display, 0, CONSOLE_LAST_ROW(current_display), CONSOLE_WIDTH(current_display));

    hv_disp_puts(cdisp, message);
    return 0;
}

int dc_bottom_menu_draw(struct DebugScreen *scr)
{
    const char copyright[] = "Copyright (C) Akos Kovacs ";
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    int x = CONSOLE_WIDTH(current_display) - sizeof(copyright);

    dc_bottom_show_message(scr, scr->dc_help);
    hv_console_set_xy(current_display, x, CONSOLE_LAST_ROW(current_display));
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_BROWN);
    hv_disp_puts(cdisp, copyright);
    return 0;
}

int dc_keyboard_handler(char scancode)
{
    int ind;
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
            ind = current_screen->dc_key % NR_SCREENS;
            dc_show_screen(ds_screen+ind);
            KBD_DELAY();
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
    int row = 5;
    const char *cmd_line = NULL;
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED);
    hv_console_set_xy(current_display, 10, row);
    hv_disp_puts(cdisp, "Version:");
    hv_console_set_xy(current_display, 30, row++);
    hv_disp_puts(cdisp, "0.1-prealpha");

    hv_console_set_xy(current_display, 10, row);
    hv_disp_puts(cdisp, "Architecture:");
    hv_console_set_xy(current_display, 30, row++);
    hv_printf(cdisp, "%d bit", sizeof(long)*8); // FIXME?

    hv_console_set_xy(current_display, 10, row);
    hv_disp_puts(cdisp, "Built at:");
    hv_console_set_xy(current_display, 30, row++);
    hv_disp_puts(cdisp, __DATE__ ", " __TIME__);

    hv_console_set_xy(current_display, 10, row);
    hv_disp_puts(cdisp, "Multiboot info:");
    if (boot_info) {
        hv_console_set_xy(current_display, 30, row++);
        hv_disp_puts(cdisp, "Valid");

        if (boot_info->flags & MB_INFO_BOOTDEV) {
            hv_console_set_xy(current_display, 10, row);
            hv_disp_puts(cdisp, "BIOS boot device:");
            hv_console_set_xy(current_display, 30, row++);
            hv_printf(cdisp, "%x", (char)boot_info->boot_device >> 24);
        }

        if (boot_info->flags & MB_INFO_BOOT_LOADER) {
            hv_console_set_xy(current_display, 10, row);
            hv_disp_puts(cdisp, "Boot loader:");
            hv_console_set_xy(current_display, 30, row++);
            hv_printf(cdisp, "%s", (const char *)boot_info->boot_loader_name);
        }

        if (boot_info->flags & MB_INFO_CMDLINE) {
            hv_console_set_xy(current_display, 10, row);
            hv_disp_puts(cdisp, "Boot command line:");
            hv_console_set_xy(current_display, 30, row++);
            cmd_line = (const char *)boot_info->cmdline;
            hv_printf(cdisp, "%s", (cmd_line && cmd_line[0]) ? cmd_line : "<none>");
        }
    } else {
        hv_console_set_xy(current_display, 30, row++);
        hv_disp_puts(cdisp, "Invalid");
    }

    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(current_display, 10, 20);
    hv_disp_puts(cdisp, "[R] - Press R to restart the computer");
    hv_console_set_xy(current_display, 10, 21);
    hv_disp_puts(cdisp, "[C] - Press C to resume loading from the first hard disk");
    return 0;    
}

static int dc_vmm_handle_key(struct DebugScreen *scr, char key)
{
    if (key == KEY_R) {
        dc_bottom_show_message(scr, "Restarting...");
        sys_reboot();
    } else if (key == KEY_C) {
        dc_bottom_show_message(scr, "Resume loading from the disk...");
        sys_chainload(); 
    }
    return 0;
}

static int dc_cpu_info_show(struct DebugScreen *scr)
{
    static char vendor[13];
    static char branding[49];
    static char *branding_start = NULL;
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;

    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED);
    hv_console_set_xy(current_display, 10, 5);
    hv_disp_puts(cdisp, "Brand name:");

    hv_console_set_xy(current_display, 30, 5);
    if (branding_start == NULL) {
        cpuid_get_branding(branding);
        branding_start = branding;
        /* The branding string is padded with spaces from
           the beginning, skip that */
        while (*branding_start == ' ') {
            branding_start++;
        }

        cpuid_get_vendor(vendor);
    }

    hv_disp_puts(cdisp, branding_start);

    hv_console_set_xy(current_display, 10, 6);
    hv_disp_puts(cdisp, "Vendor string:");
    hv_console_set_xy(current_display, 30, 6);
    hv_disp_puts(cdisp, vendor);

    return 0;
}

static int dc_cpu_handle_key(struct DebugScreen *scr, char key)
{
    return -HV_ENOIMPL;
}

/* Eh, too many entries needs scrolling */
static int ind_mmap_page = 0;
static int nr_mmap_pages = 0;
static void dc_mem_show_scrolling(void)
{
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    hv_console_set_xy(current_display, 10, 20);
    hv_printf(cdisp, "Got %d memory regions, %d shown on page (%d/%d)"
        , sys_mmap->sm_nr_maps, CONFIG_NR_MMAP_MAX_ENTRIES, ind_mmap_page+1, nr_mmap_pages);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(current_display, 10, 21);
    hv_disp_puts(cdisp, "[Space] - Press space for more entries");
}

static bool is_mmap_needs_scrolling = false;
static void mem_info_dump(struct DebugScreen *scr, struct PhysicalMMapping *phymm, int row)
{
    int i;
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    int nr_skip = ind_mmap_page*CONFIG_NR_MMAP_MAX_ENTRIES;
    struct MemoryMap *mmap = phymm->sm_maps+nr_skip;
    uint64_t size;
    
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED);
    for (i = 0; (i < nr_skip+phymm->sm_nr_maps) && (i < CONFIG_NR_MMAP_MAX_ENTRIES); i++, mmap++) {
        size = (mmap->mm_end - mmap->mm_start);
        hv_console_set_xy(current_display, 10, row);
        hv_printf(cdisp, "0x%X%X", mmap->mm_start);
        hv_console_set_xy(current_display, 25, row);
        hv_printf(cdisp, "0x%X%X", mmap->mm_end);
        hv_console_set_xy(current_display, 40, row);
        hv_printf(cdisp, "%d KB", size / 1024);
        hv_console_set_xy(current_display, 55, row++);
        hv_printf(cdisp, "[%s]", ((mmap->mm_flags & MM_RESERVED) ? "reserved" : "free"));
    }
}

static int dc_mem_info_show(struct DebugScreen *scr)
{
    struct CharacterDisplay *cdisp = (struct CharacterDisplay *)current_display;
    size_t sz_mem = 0;
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(current_display, 10, 5);
    /* Set the maximum available screen lines for the entries */
    nr_mmap_pages           = sys_mmap->sm_nr_maps / CONFIG_NR_MMAP_MAX_ENTRIES;
    /* We need one more screen page if the number of regions cannot be divided with *_MAX_ENTRIES without remainder */
    nr_mmap_pages          += (sys_mmap->sm_nr_maps % CONFIG_NR_MMAP_MAX_ENTRIES) > 0;
    is_mmap_needs_scrolling = nr_mmap_pages > 1;

    if ((boot_info != NULL) && (sys_mmap != NULL) && (boot_info->flags & MB_INFO_MEMORY) 
            && (boot_info->flags & MB_INFO_MEM_MAP)) {
        sz_mem = boot_info->mem_lower + boot_info->mem_upper;
        hv_disp_puts(cdisp, "Total memory (according to BIOS):");
        hv_console_set_xy(current_display, 45, 5);
        hv_printf(cdisp, "%d KB (%d MB)", sz_mem, sz_mem / 1024);
        mem_info_dump(scr, sys_mmap, 7);
        if (is_mmap_needs_scrolling) {
            dc_mem_show_scrolling();
        } else {
            hv_console_set_xy(current_display, 10, 20);
            hv_printf(cdisp, "Got %d memory regions ", sys_mmap->sm_nr_maps);
        }
    } else {
        hv_disp_puts(cdisp, "No memory information provided by the bootloader");
    }
    return 0;
}

static int dc_mem_handle_key(struct DebugScreen *scr, char key)
{ 
    if (is_mmap_needs_scrolling) {
        if (key == KEY_SPACE) {
            if (ind_mmap_page < nr_mmap_pages-1) {
                ind_mmap_page++;
            } else {
                ind_mmap_page = 0;
            }
            dc_show_screen(scr);
            KBD_DELAY();
        }
    }
    return 0;
}

static int dc_disk_info_show(struct DebugScreen *scr) 
{ 
    hv_console_set_xy(current_display, 26, 11);
    hv_console_set_attribute(current_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_disp_puts((struct CharacterDisplay *)current_display, "< Implementation required >");
    return -HV_ENOIMPL;
}
static int dc_disk_handle_key(struct DebugScreen *scr, char key) { return -HV_ENOIMPL; }

static int dc_guest_info_show(struct DebugScreen *scr) 
{ 
    return dc_disk_info_show(scr);
}

static int dc_guest_handle_key(struct DebugScreen *scr, char key) { return -HV_ENOIMPL; }
