/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | The actual 64-bit hypervisor payload, embedded in the main |
 * | ELF32 image.                                               |
 * +------------------------------------------------------------+
*/
#include <drivers/video/pc_console.h>
#include <drivers/comm/serial.h>
#include <print.h>
#include <system.h>

extern void os_error_stub(void);
void cpu_init_tables(void);

struct ConsoleDisplay main_display;
struct CharacterDisplay *display = (struct CharacterDisplay *)&main_display;
struct CharacterDisplay debug_serial;

void hv_start(uint32_t arg)
{
    int out_times = 10;

    /* No arguments, we must be executed from an OS, hopefully Linux :D */
#ifdef CONFIG_HV_OS_STUB
    if (arg == 0x0) {
        os_error_stub();
        return;
    }
#endif

    cpu_init_tables();
    //pic_disable();
    int_enable();

    hv_serial_init(&debug_serial);
    hv_disp_setup(&debug_serial);
    const char hello[] = "hello";
    int n = hv_printf(&debug_serial, "Hypervisor hv_start() entry at \n");
    bochs_breakpoint();
    hv_printf(&debug_serial, "hello = %s\n", hello);
    hv_printf(&debug_serial, "n = %d\n", n);
    n = hv_printf(&debug_serial, "%x%x\n", hv_start);
    hv_printf(&debug_serial, "n = %d\n", n);
    //hv_printf(&debug_serial, "Display pointer %X\n", display);

    hv_console_display_init(&main_display);
    hv_disp_setup(display);
    hv_set_stdout(display);
    hv_console_set_attribute(&main_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(&main_display, 0, 0);
    bochs_breakpoint();
    //hv_console_puts_xya(&main_display, 0, 1, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT, "HELLO!!!");

    int i = 0;
    while (i < out_times) {
        hv_disp_puts_xy(display, 0, i, "64 bit hypervisor startup...\n");
        i++;
    }

    while (1) {
        //halt();
        ;
    }
}
