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

extern void os_error_stub(void);
void cpu_init_tables(void);

struct ConsoleDisplay main_display;
struct CharacterDisplay *display = (struct CharacterDisplay *)&main_display;
struct CharacterDisplay debug_serial;

static struct VmxState vmx_state;

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

    /* Check for VT-x support */
    if (vmx_check_support() != 0) {
        hv_printf(display, "Error: VT-x not supported\n");
        goto halt;
    }

    /* Read and print VMX capability MSRs */
    if (vmx_read_capabilities(&vmx_state.vs_caps) != 0) {
        hv_printf(display, "Error: failed to read VMX capabilities\n");
        goto halt;
    }
    vmx_print_info(&vmx_state.vs_caps);

    if (!vmx_state.vs_caps.vc_unrestricted_guest) {
        hv_printf(display, "Error: unrestricted guest not supported (required for real-mode boot)\n");
        goto halt;
    }

    /* Enter VMX root operation */
    if (vmx_enable(&vmx_state) != 0) {
        hv_printf(display, "Error: VMXON failed\n");
        goto halt;
    }

    /* Build EPT and set the pointer in the VMCS */
    uint64_t eptp = ept_build();
    if (!eptp) {
        hv_printf(display, "Error: EPT build failed\n");
        goto halt;
    }

    /* Initialize VMCS with host/guest/control fields */
    if (vmcs_init(&vmx_state) != 0) {
        hv_printf(display, "Error: VMCS init failed\n");
        goto halt;
    }
    hv_printf(display, "VMCS initialized\n");

    /* Set EPT pointer now that the VMCS is current */
    if (vmx_state.vs_caps.vc_ept)
        vmx_write(VMCS_EPT_POINTER, eptp);

    hv_printf(display, "\nReady to launch guest.\n");
    hv_printf(&debug_serial, "VMX initialized, ready for VMLAUNCH\n");

halt:
    while (1)
        ;
}
