/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | The actual 64-bit hypervisor payload, embedded in the main |
 * | ELF32 image.                                               |
 * +------------------------------------------------------------+
*/
#include <drivers/video/pc_console.h>
#include <print.h>
#include <system.h>

extern void os_error_stub(void); 
static int out_times;
void cpu_init_tables(void);

static struct ConsoleDisplay main_display;

void hv_start(uint32_t arg)
{
    struct CharacterDisplay *display = (struct CharacterDisplay *)&main_display;
    out_times = 10;

    /* No arguments, we must be executed from an OS, hopefully Linux :D */
#ifdef CONFIG_HV_OS_STUB
    if (arg == 0x0) {
        os_error_stub();
        return;
    } 
#endif

    cpu_init_tables();
    hv_console_display_init(&main_display);
    hv_set_stdout(&main_display);

    int i;
    while (i < out_times) {
        hv_printf(display, "64 bit hypervisor startup...");
        i++;
    }
    
    while (1) {
        halt();
    }
}
