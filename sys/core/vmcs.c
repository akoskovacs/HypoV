/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | VMCS host/guest/control field initialization               |
 * +------------------------------------------------------------+
 */
#include <cpu.h>
#include <gdt.h>
#include <memory.h>
#include <print.h>
#include <string.h>
#include <system.h>
#include <vmx.h>

extern struct CharacterDisplay debug_serial;
extern unsigned long           __get_relocation_offset(void);

uint64_t cpu_get_gdt_base(void);
uint64_t cpu_get_idt_base(void);
uint64_t cpu_get_tss_base(void);

/* Dedicated stack for VM exit handling */
static uint8_t vm_exit_stack[8192] __align(16);

/* Write all VMCS host-state fields (our 64-bit hypervisor environment) */
static void vmcs_write_host_state(struct VmxCapabilities *caps)
{
    unsigned long rel_off = __get_relocation_offset();

    vmx_write(VMCS_HOST_CR0, cr0_read64());
    vmx_write(VMCS_HOST_CR3, cr3_read64());
    vmx_write(VMCS_HOST_CR4, cr4_read64());

    vmx_write(VMCS_HOST_CS, read_cs());
    vmx_write(VMCS_HOST_SS, read_ss());
    vmx_write(VMCS_HOST_DS, read_ds());
    vmx_write(VMCS_HOST_ES, read_ds());
    vmx_write(VMCS_HOST_FS, 0);
    vmx_write(VMCS_HOST_GS, 0);
    vmx_write(VMCS_HOST_TR, read_tr());

    vmx_write(VMCS_HOST_FS_BASE, 0);
    vmx_write(VMCS_HOST_GS_BASE, 0);
    vmx_write(VMCS_HOST_TR_BASE, cpu_get_tss_base());
    vmx_write(VMCS_HOST_GDTR_BASE, cpu_get_gdt_base());
    vmx_write(VMCS_HOST_IDTR_BASE, cpu_get_idt_base());

    vmx_write(VMCS_HOST_IA32_SYSENTER_CS, 0);
    vmx_write(VMCS_HOST_IA32_SYSENTER_ESP, 0);
    vmx_write(VMCS_HOST_IA32_SYSENTER_EIP, 0);

    vmx_write(VMCS_HOST_IA32_EFER, msr_read(MSR_IA32_EFER));
    vmx_write(VMCS_HOST_IA32_PAT, msr_read(0x277)); /* IA32_PAT */

    /* VM exit lands here with the dedicated host stack */
    vmx_write(VMCS_HOST_RSP, (uint64_t)&vm_exit_stack[sizeof(vm_exit_stack)]);
    vmx_write(VMCS_HOST_RIP, rel_off + (uint64_t)vmx_exit_trampoline);

    (void)caps;
}

/*
 * Build a real-mode segment access rights word.
 * type: 3 = data r/w accessed, 0xB = code exec/read accessed
 */
static uint32_t realmode_ar(uint32_t type)
{
    return VMCS_AR_S_BIT | VMCS_AR_PRESENT | (type & 0xF);
}

/* Write all VMCS guest-state fields.
 * If unrestricted guest is available: real mode at 0x7C00 (for OS boot).
 * Otherwise: 32-bit protected mode at 0x1000 (for vmcall proof stub). */
static void vmcs_write_guest_state(struct VmxCapabilities *caps)
{
    uint64_t guest_cr0 = msr_read(MSR_IA32_VMX_CR0_FIXED0) & msr_read(MSR_IA32_VMX_CR0_FIXED1);
    if (!caps->vc_unrestricted_guest)
        guest_cr0 |= CR0_PE; /* protected mode required without URG */
    else
        guest_cr0 &= ~(uint64_t)(CR0_PE | CR0_PG); /* real mode with URG */
    vmx_write(VMCS_GUEST_CR0, guest_cr0);
    vmx_write(VMCS_GUEST_CR3, 0);

    uint64_t guest_cr4 = msr_read(MSR_IA32_VMX_CR4_FIXED0) & msr_read(MSR_IA32_VMX_CR4_FIXED1);
    vmx_write(VMCS_GUEST_CR4, guest_cr4);
    vmx_write(VMCS_GUEST_DR7, 0x400);

    /* EFER: 0 — real mode, no long mode */
    vmx_write(VMCS_GUEST_IA32_EFER, 0);
    vmx_write(VMCS_GUEST_IA32_DEBUGCTL, 0);

    /* Real-mode code segment: CS = 0x0000, base = 0, limit = 0xFFFF */
    vmx_write(VMCS_GUEST_CS, 0x0000);
    vmx_write(VMCS_GUEST_CS_BASE, 0x0000);
    vmx_write(VMCS_GUEST_CS_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_CS_AR, realmode_ar(0xB)); /* execute/read, accessed */

    /* Real-mode data segments */
    uint32_t data_ar = realmode_ar(0x3); /* read/write, accessed */
    vmx_write(VMCS_GUEST_DS, 0);
    vmx_write(VMCS_GUEST_DS_BASE, 0);
    vmx_write(VMCS_GUEST_DS_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_DS_AR, data_ar);

    vmx_write(VMCS_GUEST_SS, 0);
    vmx_write(VMCS_GUEST_SS_BASE, 0);
    vmx_write(VMCS_GUEST_SS_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_SS_AR, data_ar);

    vmx_write(VMCS_GUEST_ES, 0);
    vmx_write(VMCS_GUEST_ES_BASE, 0);
    vmx_write(VMCS_GUEST_ES_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_ES_AR, data_ar);

    vmx_write(VMCS_GUEST_FS, 0);
    vmx_write(VMCS_GUEST_FS_BASE, 0);
    vmx_write(VMCS_GUEST_FS_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_FS_AR, data_ar);

    vmx_write(VMCS_GUEST_GS, 0);
    vmx_write(VMCS_GUEST_GS_BASE, 0);
    vmx_write(VMCS_GUEST_GS_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_GS_AR, data_ar);

    /* LDTR: unusable */
    vmx_write(VMCS_GUEST_LDTR, 0);
    vmx_write(VMCS_GUEST_LDTR_BASE, 0);
    vmx_write(VMCS_GUEST_LDTR_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_LDTR_AR, VMCS_AR_UNUSABLE);

    if (caps->vc_unrestricted_guest) {
        /* Real mode: entry at boot stub (0x8000), which loads MBR -> 0x7C00 */
        vmx_write(VMCS_GUEST_TR_AR, VMCS_AR_PRESENT | 0xB);
        vmx_write(VMCS_GUEST_TR_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_IDTR_LIMIT, 0x03FF);
        vmx_write(VMCS_GUEST_RIP, 0x8000);
        vmx_write(VMCS_GUEST_RSP, 0x7C00);
    } else {
        /* 64-bit long mode: share host's identity page tables, vmcall stub at 0x1000.
         * FIXED0 forces PG=1 so we need valid page tables — use the host's. */
        uint64_t cr4 = msr_read(MSR_IA32_VMX_CR4_FIXED0) & msr_read(MSR_IA32_VMX_CR4_FIXED1);
        cr4 |= CR4_PAE; /* PAE required for long mode */
        vmx_write(VMCS_GUEST_CR4, cr4);
        vmx_write(VMCS_GUEST_CR3, cr3_read64()); /* host's identity page tables */
        vmx_write(VMCS_GUEST_IA32_EFER, EFER_LME | EFER_LMA | EFER_SCE);
        /* 64-bit code segment */
        vmx_write(VMCS_GUEST_CS_AR, VMCS_AR_S_BIT | VMCS_AR_PRESENT |
                                        VMCS_AR_L_BIT | 0xB); /* L=1 for 64-bit */
        vmx_write(VMCS_GUEST_CS_LIMIT, 0xFFFF);
        /* 64-bit data segments (same as 32-bit for data) */
        uint32_t d64ar = VMCS_AR_S_BIT | VMCS_AR_PRESENT | 0x3;
        vmx_write(VMCS_GUEST_DS_AR, d64ar);
        vmx_write(VMCS_GUEST_DS_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_SS_AR, d64ar);
        vmx_write(VMCS_GUEST_SS_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_ES_AR, d64ar);
        vmx_write(VMCS_GUEST_ES_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_FS_AR, d64ar);
        vmx_write(VMCS_GUEST_FS_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_GS_AR, d64ar);
        vmx_write(VMCS_GUEST_GS_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_TR_AR, VMCS_AR_PRESENT | 0xB); /* 64-bit TSS busy */
        vmx_write(VMCS_GUEST_TR_LIMIT, 0x67);
        vmx_write(VMCS_GUEST_IDTR_LIMIT, 0xFFFF);
        vmx_write(VMCS_GUEST_RIP, 0x1000); /* vmcall proof stub */
        vmx_write(VMCS_GUEST_RSP, 0x7000);
    }
    vmx_write(VMCS_GUEST_TR, 0);
    vmx_write(VMCS_GUEST_TR_BASE, 0);
    vmx_write(VMCS_GUEST_GDTR_BASE, 0);
    vmx_write(VMCS_GUEST_GDTR_LIMIT, 0xFFFF);
    vmx_write(VMCS_GUEST_IDTR_BASE, 0);
    vmx_write(VMCS_GUEST_RFLAGS, 0x2);

    vmx_write(VMCS_GUEST_IA32_SYSENTER_CS, 0);
    vmx_write(VMCS_GUEST_IA32_SYSENTER_ESP, 0);
    vmx_write(VMCS_GUEST_IA32_SYSENTER_EIP, 0);

    vmx_write(VMCS_GUEST_ACTIVITY_STATE, 0); /* active */
    vmx_write(VMCS_GUEST_INTERRUPTIBILITY, 0);
    vmx_write(VMCS_GUEST_PENDING_DBG_EXCEPT, 0);

    /* Required: VMCS link pointer must be 0xFFFF...F (no shadow VMCS) */
    vmx_write(VMCS_VMCS_LINK_PTR, VMCS_LINK_PTR_INVALID);

    (void)caps;
}

/* Write all VMCS control fields */
static void vmcs_write_controls(struct VmxCapabilities *caps)
{
    vmx_write(VMCS_PIN_BASED_CTLS, caps->vc_pin_ctls);
    vmx_write(VMCS_CPU_BASED_CTLS, caps->vc_proc_ctls);
    if (caps->vc_proc_ctls & PROC_ACTIVATE_SECONDARY) {
        vmx_write(VMCS_CPU_BASED_CTLS2, caps->vc_proc_ctls2);
    }
    vmx_write(VMCS_VM_EXIT_CTLS, caps->vc_exit_ctls);
    vmx_write(VMCS_VM_ENTRY_CTLS, caps->vc_entry_ctls);

    /* IA-32e guest mode: set for 64-bit proof stub, clear for real mode boot */
    uint32_t entry_ctls = caps->vc_unrestricted_guest
                              ? (caps->vc_entry_ctls & ~ENTRY_IA32E_GUEST)
                              : (caps->vc_entry_ctls | ENTRY_IA32E_GUEST);
    vmx_write(VMCS_VM_ENTRY_CTLS, entry_ctls);

    /* Exception bitmap: 0 — let all exceptions pass through to guest */
    vmx_write(VMCS_EXCEPTION_BITMAP, 0);

    /* No page-fault filtering */
    vmx_write(VMCS_PF_ERROR_CODE_MASK, 0);
    vmx_write(VMCS_PF_ERROR_CODE_MATCH, 0);

    /* No CR3 targets */
    vmx_write(VMCS_CR3_TARGET_COUNT, 0);
    vmx_write(VMCS_CR3_TARGET0, 0);
    vmx_write(VMCS_CR3_TARGET1, 0);
    vmx_write(VMCS_CR3_TARGET2, 0);
    vmx_write(VMCS_CR3_TARGET3, 0);

    /* CR0/CR4 masks: don't intercept guest changes to most bits */
    vmx_write(VMCS_CR0_GUEST_HOST_MASK, 0);
    vmx_write(VMCS_CR4_GUEST_HOST_MASK, 0);
    vmx_write(VMCS_CR0_READ_SHADOW, 0);
    vmx_write(VMCS_CR4_READ_SHADOW, 0);

    /* TSC offset: 0 — Phase 4 will handle RDTSC exits */
    vmx_write(VMCS_TSC_OFFSET, 0);

    /* MSR load/store counts: 0 — no auto-load/store on exit/entry */
    vmx_write(VMCS_VM_EXIT_MSR_STORE_COUNT, 0);
    vmx_write(VMCS_VM_EXIT_MSR_LOAD_COUNT, 0);
    vmx_write(VMCS_VM_ENTRY_MSR_LOAD_COUNT, 0);

    /* VPID: 1 (host is 0, each guest gets a unique ID) */
    if (caps->vc_vpid)
        vmx_write(VMCS_VPID, 1);

    /* EPT: build and set EPT pointer (AD disabled for compatibility with
     * Bochs and older CPUs reporting AD support but rejecting EPTP AD bit) */
    if (caps->vc_ept) {
        uint64_t eptp = ept_build(false);
        vmx_write(VMCS_EPT_POINTER, eptp);
    }
}

int vmcs_init(struct VmxState *state)
{
    uint64_t vmcs_pa = (uint64_t)state->vs_vmcs; /* VA == PA, identity mapped */

    /* VMCLEAR initializes the VMCS and sets it as not-current */
    if (vmx_clear(&vmcs_pa) != 0) {
        hv_printf(&debug_serial, "VMCS: VMCLEAR failed\n");
        return -1;
    }

    /* VMPTRLD makes this VMCS current */
    if (vmx_ptr_load(&vmcs_pa) != 0) {
        hv_printf(&debug_serial, "VMCS: VMPTRLD failed\n");
        return -1;
    }

    vmcs_write_host_state(&state->vs_caps);
    vmcs_write_guest_state(&state->vs_caps);
    vmcs_write_controls(&state->vs_caps);

    hv_printf(&debug_serial, "VMCS: initialized\n");
    return 0;
}
