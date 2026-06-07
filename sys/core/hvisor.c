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
#include <cpu.h>
#include <interrupt.h>
#include <memory.h>
#include <print.h>
#include <system.h>
#include <vmx.h>
#include <svm.h>
#include <hv_ops.h>

extern void os_error_stub(void);
void cpu_init_tables(void);
void cpu_init_host_paging(void);
extern unsigned long __get_relocation_offset(void);

struct ConsoleDisplay main_display;
struct CharacterDisplay *display = (struct CharacterDisplay *)&main_display;
struct CharacterDisplay debug_serial;

/* ABI-safe memory layout info handed over by the 32bit loader (see
 * struct HvBootHandoff) — tells the NPT which physical memory belongs
 * to the hypervisor itself and must be hidden from the guest. */
struct HvBootHandoff *hv_boot_handoff = NULL;

extern struct VmxState vmx_state;
extern struct SvmState svm_state;

const struct HvOperations *hv_detect_backend(void)
{
    int32_t regs[4];

    /* Check CPU vendor first — hypervisors (e.g. Hyper-V on Azure) may set
     * vendor-specific CPUID bits that look like VMX/SVM on the wrong vendor. */
    cpuid(0x0, regs);
    int32_t vendor_ebx = regs[1]; /* "Genu" for Intel, "Auth" for AMD */

    if (vendor_ebx == 0x756e6547)
    { /* Intel: "GenuineIntel" */
        cpuid(0x1, regs);
        if (regs[CPUID_REG_ECX] & CPUID_FEATURE_VMX)
            return &vmx_ops;
    }
    else if (vendor_ebx == 0x68747541)
    { /* AMD: "AuthenticAMD" */
        cpuid(0x80000001, regs);
        if (regs[CPUID_REG_ECX] & CPUID_EXT_FEATURE_SVM)
            return &svm_ops;
    }

    hv_printf(&debug_serial, "CPU vendor=%x — no VMX/SVM\n", vendor_ebx);
    return NULL;
}

void hv_start(uint32_t arg)
{
#ifdef CONFIG_HV_OS_STUB
    if (arg == 0x0)
    {
        os_error_stub();
        return;
    }
#endif

    /* The loader passes the physical address of its HvBootHandoff (an
     * ABI-safe, pointer-free struct — see memory.h) instead of its
     * SystemInfo, so the NPT can later hide HypoV's own low-memory
     * footprint and the BIOS-reserved regions from the guest. */
    hv_boot_handoff = (arg != 0x0) ? (struct HvBootHandoff *)(unsigned long)arg : NULL;

    cpu_init_tables();

    /* Switch to hvcore's own page tables *before* anything SVM/VMX-related
     * runs — in particular before the very first VMRUN. From here on, the
     * host CR3 that VMRUN/#VMEXIT auto-saves and restores always points at
     * memory the guest's flat-identity-mapped NPT view can never reach, so
     * a guest scribbling over "its" RAM can no longer corrupt the page
     * tables the host depends on. See docs/SVM_HOST_PAGING_FIX.md. */
    cpu_init_host_paging();

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
    hv_printf(&debug_serial, "Host paging: CR3=%x (hvcore-private)\n", cr3_read64());

    /* Fill ops structs via code — static .data initializers are not
     * reliably loaded by the 32-bit ELF bootstrap. */
    vmx_backend_init(&vmx_ops);
    svm_backend_init(&svm_ops);

    const struct HvOperations *ops = hv_detect_backend();
    if (!ops)
    {
        hv_printf(display, "Error: no hardware virtualization support\n");
        hv_printf(&debug_serial, "No VMX or SVM found\n");
        goto halt;
    }

    hv_printf(display, "Backend: %s\n", ops->name);
    hv_printf(&debug_serial, "Backend: %s\n", ops->name);

    if (ops->check_support() != 0)
        goto halt;

    if (ops->enable() != 0)
    {
        hv_printf(display, "Error: backend enable failed\n");
        goto halt;
    }

    ops->print_info();

    ops->run_guest(); /* does not return on success */

halt:
    while (1)
        __asm__ __volatile__("hlt");
}
