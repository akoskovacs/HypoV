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
#include <interrupt.h>
#include <print.h>
#include <system.h>
#include <vmx.h>
#include <svm.h>
#include <hv_ops.h>

extern void os_error_stub(void);
void cpu_init_tables(void);

struct ConsoleDisplay  main_display;
struct CharacterDisplay *display = (struct CharacterDisplay *)&main_display;
struct CharacterDisplay  debug_serial;

extern struct VmxState vmx_state;
extern struct SvmState svm_state;

const struct HvOperations *hv_detect_backend(void)
{
    int32_t regs[4];

    cpuid(0x1, regs);
    if (regs[CPUID_REG_ECX] & (1 << 5))
        return &vmx_ops;   /* Intel VT-x */

    cpuid(0x80000001, regs);
    if (regs[CPUID_REG_ECX] & (1 << 2))
        return &svm_ops;   /* AMD SVM */

    return NULL;
}

void hv_start(uint32_t arg)
{
#ifdef CONFIG_HV_OS_STUB
    if (arg == 0x0) {
        os_error_stub();
        return;
    }
#endif

    cpu_init_tables();
    pic_init(0x20, 0x28);
    int_enable();

    hv_serial_init(&debug_serial);
    hv_disp_setup(&debug_serial);
    hv_console_display_init(&main_display);
    hv_disp_setup(display);
    hv_set_stdout(display);
    hv_console_set_attribute(&main_display, FG_COLOR_WHITE | BG_COLOR_RED | LIGHT);
    hv_console_set_xy(&main_display, 0, 0);

    hv_printf(display, "HypoV 64-bit hypervisor core\n\n");
    hv_printf(&debug_serial, "HypoV hv_start()\n");

    const struct HvOperations *ops = hv_detect_backend();
    if (!ops) {
        hv_printf(display, "Error: no hardware virtualization support\n");
        hv_printf(&debug_serial, "No VMX or SVM found\n");
        goto halt;
    }

    hv_printf(display, "Backend: %s\n", ops->name);

    if (ops->check_support() != 0)
        goto halt;

    ops->print_info();

    if (ops->enable() != 0) {
        hv_printf(display, "Error: backend enable failed\n");
        goto halt;
    }

    ops->run_guest();  /* does not return on success */

halt:
    while (1)
        ;
}
