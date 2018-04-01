/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | The actual 64-bit hypervisor payload, embedded in the main |
 * | ELF32 image.                                               |
 * +------------------------------------------------------------+
*/
#include <drivers/video/pc_console.h>
#include <system.h>

extern void os_error_stub(void); 
static int out_times;
void cpu_init_tables(void);

void hv_start(uint32_t arg)
{
    const char *message = "Welcome in 64 bit land :D :D :D";
    const char *hello = message;
    out_times = 10;

    /* No arguments, we must be executed from an OS, hopefully Linux :D */
#ifdef CONFIG_HV_OS_STUB
    if (arg == 0x0) {
        os_error_stub();
        return;
    } 
#endif

    console_font_t font = BG_COLOR_CYAN | FG_COLOR_WHITE | LIGHT;
    volatile console_font_t *videoram = PC_VIDEORAM_BASE_ADDRESS;
    /* Clear screen without 32bit code */
    for (int i = 0; i < CONFIG_CONSOLE_WIDTH * CONFIG_CONSOLE_HEIGHT; i++) {
       videoram[i] = 0 | (font << 8);
    }

    int i, j;
    i = j = 0;
    while (j < out_times) {
        hello = message;
        i = 0;
        while (*hello) {
            videoram[(j * CONFIG_CONSOLE_WIDTH) + (i++)] = *hello++ | (font << 8);
        }
        j++;
    }
    cpu_init_tables();
    
    while (1) {
        halt();
    }
}
