/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | Intel VT-x initialization: VMXON, capability detection     |
 * +------------------------------------------------------------+
 */
#include <cpu.h>
#include <system.h>
#include <memory.h>
#include <print.h>
#include <string.h>
#include <vmx.h>
#include <hv_ops.h>

extern struct CharacterDisplay debug_serial;
extern struct CharacterDisplay *display;

struct VmxState vmx_state;

/* Static 4K-aligned regions — no 32-bit heap allocator available in hvcore */
static struct VmxonRegion vmxon_region __aligned_4k;
static uint8_t vmcs_region[VMX_REGION_SIZE] __aligned_4k;

/*
 * Adjust a control field to satisfy the allowed-0 / allowed-1 constraints
 * from the given capability MSR.
 *
 * Bits that are 1 in the low 32 bits of the MSR must be 1 in the control.
 * Bits that are 0 in the high 32 bits of the MSR must be 0 in the control.
 */
static uint32_t vmx_adjust_controls(uint32_t requested, uint32_t msr_index)
{
    uint64_t cap = msr_read(msr_index);
    uint32_t must1 = (uint32_t)(cap & 0xFFFFFFFF); /* forced-1 bits */
    uint32_t may1 = (uint32_t)(cap >> 32);         /* allowed-1 bits */
    return (requested | must1) & may1;
}

int vmx_check_support(void)
{
    int32_t regs[4];
    cpuid(0x1, regs);
    /* ECX bit 5 = VMX (matches CPU_FEATURE_VMX which is bit 37 of the combined flag) */
    if (!(regs[CPUID_REG_ECX] & (1 << 5)))
    {
        hv_printf(&debug_serial, "VMX: not supported on this CPU\n");
        hv_printf(display, "VMX: not supported\n");
        return -1;
    }
    return 0;
}

int vmx_read_capabilities(struct VmxCapabilities *caps)
{
    uint64_t basic = msr_read(MSR_IA32_VMX_BASIC);

    caps->vc_revision_id = (uint32_t)(basic & VMX_BASIC_REVISION_MASK);
    caps->vc_vmcs_size = (uint16_t)((basic >> VMX_BASIC_VMCS_SIZE_SHIFT) & VMX_BASIC_VMCS_SIZE_MASK);
    caps->vc_true_ctls = !!(basic & VMX_BASIC_TRUE_CTLS);

    /* Use TRUE MSRs if available, for more flexible control adjustment */
    uint32_t pin_msr = caps->vc_true_ctls ? MSR_IA32_VMX_TRUE_PINBASED : MSR_IA32_VMX_PINBASED_CTLS;
    uint32_t proc_msr = caps->vc_true_ctls ? MSR_IA32_VMX_TRUE_PROCBASED : MSR_IA32_VMX_PROCBASED_CTLS;
    uint32_t exit_msr = caps->vc_true_ctls ? MSR_IA32_VMX_TRUE_EXIT : MSR_IA32_VMX_EXIT_CTLS;
    uint32_t entry_msr = caps->vc_true_ctls ? MSR_IA32_VMX_TRUE_ENTRY : MSR_IA32_VMX_ENTRY_CTLS;

    /* Compute the adjusted control values we want */
    caps->vc_pin_ctls = vmx_adjust_controls(
        PIN_EXT_INT_EXITING | PIN_NMI_EXITING, pin_msr);

    caps->vc_proc_ctls = vmx_adjust_controls(
        PROC_RDTSC_EXITING | PROC_HLT_EXITING | PROC_ACTIVATE_SECONDARY, proc_msr);

    /* Check secondary controls availability */
    caps->vc_proc_ctls2 = 0;
    if (caps->vc_proc_ctls & PROC_ACTIVATE_SECONDARY)
    {
        uint64_t proc2_cap = msr_read(MSR_IA32_VMX_PROCBASED_CTLS2);
        uint32_t proc2_may1 = (uint32_t)(proc2_cap >> 32);

        caps->vc_ept = !!(proc2_may1 & PROC2_EPT);
        caps->vc_vpid = !!(proc2_may1 & PROC2_VPID);
        caps->vc_unrestricted_guest = !!(proc2_may1 & PROC2_UNRESTRICTED_GUEST);

        uint32_t wanted2 = 0;
        if (caps->vc_ept)
            wanted2 |= PROC2_EPT;
        if (caps->vc_vpid)
            wanted2 |= PROC2_VPID;
        if (caps->vc_unrestricted_guest)
            wanted2 |= PROC2_UNRESTRICTED_GUEST;
        if (proc2_may1 & PROC2_RDTSCP)
            wanted2 |= PROC2_RDTSCP;

        caps->vc_proc_ctls2 = vmx_adjust_controls(wanted2, MSR_IA32_VMX_PROCBASED_CTLS2);
    }
    else
    {
        caps->vc_ept = false;
        caps->vc_vpid = false;
        caps->vc_unrestricted_guest = false;
    }

    caps->vc_exit_ctls = vmx_adjust_controls(
        EXIT_HOST_64BIT | EXIT_SAVE_EFER | EXIT_LOAD_EFER, exit_msr);

    caps->vc_entry_ctls = vmx_adjust_controls(
        ENTRY_LOAD_EFER, entry_msr);

    return 0;
}

void vmx_print_info(struct VmxCapabilities *caps)
{
    hv_printf(display, "VMX revision:       %x\n", caps->vc_revision_id);
    hv_printf(display, "VMCS size:          %d bytes\n", caps->vc_vmcs_size);
    hv_printf(display, "TRUE MSRs:          %s\n", caps->vc_true_ctls ? "yes" : "no");
    hv_printf(display, "EPT:                %s\n", caps->vc_ept ? "yes" : "no");
    hv_printf(display, "VPID:               %s\n", caps->vc_vpid ? "yes" : "no");
    hv_printf(display, "Unrestricted guest: %s\n", caps->vc_unrestricted_guest ? "yes" : "no");
    hv_printf(display, "Pin controls:       %x\n", caps->vc_pin_ctls);
    hv_printf(display, "Proc controls:      %x\n", caps->vc_proc_ctls);
    hv_printf(display, "Proc controls2:     %x\n", caps->vc_proc_ctls2);
    hv_printf(display, "Exit controls:      %x\n", caps->vc_exit_ctls);
    hv_printf(display, "Entry controls:     %x\n", caps->vc_entry_ctls);

    hv_printf(&debug_serial, "VMX revision %x, VMCS %d bytes, EPT=%d VPID=%d URG=%d\n",
              caps->vc_revision_id, caps->vc_vmcs_size,
              caps->vc_ept, caps->vc_vpid, caps->vc_unrestricted_guest);
}

int vmx_enable(struct VmxState *state)
{
    struct VmxCapabilities *caps = &state->vs_caps;

    /* Use statically allocated 4K-aligned regions (no 32-bit heap in hvcore) */
    bzero(&vmxon_region, VMX_REGION_SIZE);
    vmxon_region.vmxon_revision_id = caps->vc_revision_id;
    state->vs_vmxon = &vmxon_region;

    bzero(vmcs_region, VMX_REGION_SIZE);
    *(uint32_t *)vmcs_region = caps->vc_revision_id;
    state->vs_vmcs = vmcs_region;

    /* Apply CR0 fixed bits required by VMX */
    uint64_t cr0 = cr0_read64();
    cr0 |= msr_read(MSR_IA32_VMX_CR0_FIXED0);
    cr0 &= msr_read(MSR_IA32_VMX_CR0_FIXED1);
    cr0_write64(cr0);

    /* Apply CR4 fixed bits and enable VMXE */
    uint64_t cr4 = cr4_read64();
    cr4 |= CR4_VMXE;
    cr4 |= msr_read(MSR_IA32_VMX_CR4_FIXED0);
    cr4 &= msr_read(MSR_IA32_VMX_CR4_FIXED1);
    cr4_write64(cr4);

    /* Verify VMXE was actually accepted (KVM may silently drop the bit) */
    uint64_t cr4_actual = cr4_read64();
    hv_printf(&debug_serial, "VMX: CR4=%x VMXE=%d feature_ctl=%x\n",
              (unsigned)cr4_actual, !!(cr4_actual & CR4_VMXE),
              (unsigned)msr_read(0x3A /* IA32_FEATURE_CONTROL */));
    if (!(cr4_actual & CR4_VMXE)) {
        hv_printf(&debug_serial, "VMX: CR4.VMXE not set — nested VMX unavailable\n");
        hv_printf(display, "VMX: CR4.VMXE not set\n");
        return -1;
    }

    /* Enter VMX root operation */
    uint64_t vmxon_pa = (uint64_t)state->vs_vmxon; /* identity mapped: VA == PA */
    if (vmx_on(&vmxon_pa) != 0)
    {
        hv_printf(&debug_serial, "VMX: VMXON failed\n");
        hv_printf(display, "VMX: VMXON failed\n");
        /* Clear VMXE on failure */
        cr4_write64(cr4_read64() & ~(uint64_t)CR4_VMXE);
        return -1;
    }

    hv_printf(&debug_serial, "VMX: VMXON succeeded\n");
    hv_printf(display, "VMX: active\n");
    return 0;
}

/* HvOperations backend for Intel VT-x */

static int  vmx_ops_enable(void)
{
    if (vmx_read_capabilities(&vmx_state.vs_caps) != 0) return -1;
    return vmx_enable(&vmx_state);
}
static void vmx_ops_print_info(void) { vmx_print_info(&vmx_state.vs_caps); }
static void vmx_ops_run_guest(void)
{
    if (vmx_state.vs_caps.vc_unrestricted_guest) {
        /* Real-mode guest: copy boot stub to 0x8000, VMCS sets RIP=0x7C00 */
        #include "boot_stub.h"
        volatile uint8_t *dst = (volatile uint8_t *)0x8000;
        const uint8_t    *src = boot_stub_bytes;
        unsigned int      i;
        for (i = 0; i < boot_stub_bytes_len; i++)
            dst[i] = src[i];
    } else {
        /* Fallback: 64-bit vmcall proof stub at guest physical 0x1000 */
        volatile uint8_t *s = (volatile uint8_t *)0x1000;
        uint32_t sig = (uint32_t)HV_HYPERCALL_SIGNATURE;
        s[0]=0xB8;                              /* mov eax, imm32 */
        s[1]=(sig>>0)&0xFF; s[2]=(sig>>8)&0xFF;
        s[3]=(sig>>16)&0xFF; s[4]=(sig>>24)&0xFF;
        s[5]=0x0F; s[6]=0x01; s[7]=0xC1;       /* vmcall */
        s[8]=0xF4;                              /* hlt */
        s[9]=0xEB; s[10]=0xFD;                 /* jmp $-1 */
    }
    vmcs_init(&vmx_state);
    hv_printf(&debug_serial, "VMX: launching guest\n");
    vmx_launch();
}

/* Zero-initialized (.bss) — filled at runtime by vmx_backend_init()
 * to avoid relying on .data static initializers which the ELF loader
 * may not populate correctly. */
struct HvOperations vmx_ops;

void vmx_backend_init(struct HvOperations *ops)
{
    ops->name          = "Intel VT-x";
    ops->check_support = vmx_check_support;
    ops->print_info    = vmx_ops_print_info;
    ops->enable        = vmx_ops_enable;
    ops->run_guest     = vmx_ops_run_guest;
}
