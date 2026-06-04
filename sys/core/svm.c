/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | AMD SVM: enable, VMCB init, guest run loop, exit handler   |
 * +------------------------------------------------------------+
*/
#include <system.h>
#include <cpu.h>
#include <memory.h>
#include <print.h>
#include <string.h>
#include <svm.h>
#include <hv_ops.h>

extern struct CharacterDisplay debug_serial;
extern struct CharacterDisplay *display;

struct SvmState svm_state;

/* 4KB host save area — written to MSR_VM_HSAVE_PA */
static uint8_t svm_host_save[PAGE_SIZE_4K] __aligned_4k;

/* Static VMCB (4KB aligned) */
static struct Vmcb svm_vmcb __aligned_4k;

static void seg_set(struct SvmSegment *s, uint16_t sel, uint64_t base,
                    uint32_t limit, uint16_t attrib)
{
    s->sel    = sel;
    s->base   = base;
    s->limit  = limit;
    s->attrib = attrib;
}

static void vmcb_init_guest_state(struct Vmcb *v)
{
    struct SvmStateSave *ss = &v->state;

    /* Entry at 0x8000: a tiny stub loads the HDD MBR via INT 13h.
     * INT 13h is already wired up by the BIOS before HypoV loaded. */
    seg_set(&ss->cs,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_CODE);
    seg_set(&ss->ds,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    seg_set(&ss->ss,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    seg_set(&ss->es,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    seg_set(&ss->fs,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    seg_set(&ss->gs,   0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    seg_set(&ss->ldtr, 0, 0, 0xFFFF, SVM_ATTRIB_LDTR);
    seg_set(&ss->tr,   0, 0, 0xFFFF, SVM_ATTRIB_TR);

    /* GDTR / IDTR: real-mode IVT at 0 */
    ss->gdtr.base  = 0; ss->gdtr.limit = 0xFFFF;
    ss->idtr.base  = 0; ss->idtr.limit = 0x03FF;

    /* SVME must be set in guest EFER per AMD APM and QEMU validation */
    ss->efer   = EFER_SVME;
    /* CR0 reset value: ET=1 (bit 4), PE=0, PG=0 — real mode */
    ss->cr0    = (1ULL << 4);
    ss->cr3    = 0;
    ss->cr4    = 0;
    ss->dr6    = 0xFFFF0FF0; /* reserved bits must be 1 (AMD APM reset value) */
    ss->dr7    = 0x400;
    ss->rflags = 0x2;    /* reserved bit always 1 */
    ss->rip    = 0x8000; /* HDD boot stub — loads MBR via INT 13h */
    ss->rsp    = 0x7C00;
    ss->rax    = 0;
    ss->cpl    = 0;
}

static void vmcb_init_controls(struct Vmcb *v, uint64_t npt_root)
{
    struct SvmControl *c = &v->control;

    c->guest_asid = 1;   /* ASID 0 is reserved for host */

    c->icept_misc1 = SVM_ICEPT_CPUID   |
                     SVM_ICEPT_RDTSC   |
                     SVM_ICEPT_HLT     |
                     SVM_ICEPT_IOIO    |
                     SVM_ICEPT_SHUTDOWN;

    c->icept_misc2 = SVM_ICEPT_VMRUN   |  /* always intercept nested VMRUN */
                     SVM_ICEPT_VMMCALL |
                     SVM_ICEPT_RDTSCP;

    /* Enable Nested Page Tables */
    c->np_enable = 1;
    c->n_cr3     = npt_root;
}

/* -----------------------------------------------------------------------
 * Exit handler
 * ----------------------------------------------------------------------- */

static void svm_skip_instruction(struct Vmcb *v)
{
    /* Prefer NEXT_RIP (nrip-save CPU feature, VMCB control offset 0xC8) */
    uint64_t next_rip = *(uint64_t *)((uint8_t *)&v->control + 0xC8);
    if (next_rip) {
        v->state.rip = next_rip;
        return;
    }

    /* For IOIO exits, exit_info2 holds the next sequential RIP */
    if (v->control.exit_code == SVM_EXIT_IOIO) {
        v->state.rip = v->control.exit_info2;
        return;
    }

    /* Known instruction lengths — avoids rip++ corrupting the guest
     * when nrip-save is unavailable (e.g. QEMU TCG).               */
    uint32_t len;
    switch (v->control.exit_code) {
    case SVM_EXIT_CPUID:    len = 2; break;  /* 0F A2 */
    case SVM_EXIT_RDTSC:    len = 2; break;  /* 0F 31 */
    case SVM_EXIT_RDTSCP:   len = 3; break;  /* 0F 01 F9 */
    case SVM_EXIT_HLT:      len = 1; break;  /* F4 */
    case SVM_EXIT_VMMCALL:  len = 3; break;  /* 0F 01 D9 */
    case 0x6F:              len = 2; break;  /* RDPMC: 0F 33 */
    default:                len = 1; break;
    }
    v->state.rip += len;
}

static void handle_svm_cpuid(struct Vmcb *v, struct GuestRegisters *regs)
{
    int32_t out[4];
    uint32_t leaf    = (uint32_t)regs->rax;
    uint32_t subleaf = (uint32_t)regs->rcx;

    __asm__ __volatile__(
        "cpuid"
        : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
        : "a"(leaf), "c"(subleaf)
    );

    switch (leaf) {
    case 0x1:
        out[2] &= ~(1 << 31); /* mask hypervisor-present bit */
        break;
    case 0x80000001:
        out[2] &= ~(1 << 2);  /* mask SVM bit for transparency */
        break;
    case 0x40000000:
        out[0] = out[1] = out[2] = out[3] = 0;
        break;
    }

    regs->rax = (uint64_t)(uint32_t)out[0];
    regs->rbx = (uint64_t)(uint32_t)out[1];
    regs->rcx = (uint64_t)(uint32_t)out[2];
    regs->rdx = (uint64_t)(uint32_t)out[3];
    v->state.rax = regs->rax;
    svm_skip_instruction(v);
}

static void handle_svm_rdtsc(struct Vmcb *v, struct GuestRegisters *regs)
{
    uint32_t lo, hi;
    rdtsc(&hi, &lo);
    regs->rax    = lo;
    regs->rdx    = hi;
    v->state.rax = lo;
    svm_skip_instruction(v);
}

static void handle_svm_vmmcall(struct Vmcb *v, struct GuestRegisters *regs,
                                uint64_t *exit_count)
{
    if (regs->rax == HV_HYPERCALL_SIGNATURE) {
        regs->rbx = HV_HYPERCALL_MAGIC;
        regs->rcx = *exit_count;
        hv_printf(&debug_serial, "VMMCALL: HypoV hypercall OK! magic=%x exit_count=%d\n",
                  HV_HYPERCALL_MAGIC, *exit_count);
        hv_printf(display, "VMMCALL: guest confirmed running under HypoV!\n");
    } else {
        regs->rbx = 0;
        regs->rcx = 0;
    }
    v->state.rax = regs->rax;
    svm_skip_instruction(v);
}

static void handle_svm_io(struct Vmcb *v, struct GuestRegisters *regs)
{
    uint64_t info = v->control.exit_info1;
    uint16_t port = (uint16_t)(info >> SVM_IOIO_PORT_SHIFT);
    int      is_in = (int)(info & SVM_IOIO_TYPE_IN);

    if (is_in) {
        uint64_t val = 0xFFFFFFFF;
        if      (info & SVM_IOIO_SZ8)  val = inb(port);
        else if (info & SVM_IOIO_SZ16) val = inw(port);
        regs->rax    = val;
        v->state.rax = val;
    } else {
        if      (info & SVM_IOIO_SZ8)  outb(port, (uint8_t)regs->rax);
        else if (info & SVM_IOIO_SZ16) outw(port, (uint16_t)regs->rax);
    }
    svm_skip_instruction(v);
}

static void svm_exit_dispatch(struct Vmcb *v, struct GuestRegisters *regs,
                              uint64_t *exit_count)
{
    uint64_t code = v->control.exit_code;

    (*exit_count)++;

    switch (code) {
    case SVM_EXIT_CPUID:
        handle_svm_cpuid(v, regs);
        break;
    case SVM_EXIT_RDTSC:
    case SVM_EXIT_RDTSCP:
        handle_svm_rdtsc(v, regs);
        break;
    case SVM_EXIT_VMMCALL:
        handle_svm_vmmcall(v, regs, exit_count);
        break;
    case SVM_EXIT_IOIO:
        handle_svm_io(v, regs);
        break;
    case SVM_EXIT_HLT:
        svm_skip_instruction(v);
        break;
    case SVM_EXIT_NPF: {
        uint64_t gpa = v->control.exit_info2;
        uint64_t gip = v->state.rip;
        hv_printf(&debug_serial, "NPF: GPA=%x GIP=%x\n", gpa, gip);
        hv_printf(display, "NPF GPA=%x\n", gpa);
        goto halt;
    }
    case SVM_EXIT_SHUTDOWN:
        hv_printf(&debug_serial, "Guest shutdown: RIP=%x CR0=%x\n",
                  v->state.rip, v->state.cr0);
        hv_printf(display, "Guest shutdown RIP=%x\n", v->state.rip);
        goto halt;
    default:
        hv_printf(&debug_serial, "Unhandled SVM exit: code=%x RIP=%x\n",
                  code, v->state.rip);
        svm_skip_instruction(v);
        break;
    }
    return;
halt:
    while (1)
        ;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

int svm_check_support(void)
{
    int32_t regs[4];
    cpuid(0x80000001, regs);
    if (!(regs[CPUID_REG_ECX] & (1 << 2))) {
        hv_printf(&debug_serial, "SVM: not supported on this CPU\n");
        hv_printf(display, "SVM: not supported\n");
        return -1;
    }
    /* Check BIOS hasn't disabled SVM via VM_CR.SVMDIS */
    if (msr_read(MSR_VM_CR) & VM_CR_SVMDIS) {
        hv_printf(&debug_serial, "SVM: disabled in BIOS (VM_CR.SVMDIS)\n");
        hv_printf(display, "SVM: disabled in BIOS\n");
        return -1;
    }
    return 0;
}

void svm_print_info(void)
{
    hv_printf(display, "SVM: supported\n");
    hv_printf(display, "NPT: %dMB mapped\n", 512);
    hv_printf(&debug_serial, "SVM: enabled, VMCB at %x, host_save at %x\n",
              &svm_vmcb, svm_host_save);
}

int svm_enable(struct SvmState *state)
{
    /* Enable SVM in EFER */
    msr_write(MSR_IA32_EFER, msr_read(MSR_IA32_EFER) | EFER_SVME);

    /* Register host save area */
    msr_write(MSR_VM_HSAVE_PA, (uint64_t)svm_host_save);

    /* Build NPT */
    uint64_t npt_root = npt_build();

    /* Initialize VMCB */
    bzero(&svm_vmcb, sizeof(svm_vmcb));
    vmcb_init_controls(&svm_vmcb, npt_root);
    vmcb_init_guest_state(&svm_vmcb);

    state->ss_vmcb = &svm_vmcb;
    state->ss_exit_count = 0;
    bzero(&state->ss_guest_regs, sizeof(state->ss_guest_regs));

    hv_printf(&debug_serial, "SVM: active\n");
    hv_printf(display, "SVM: active\n");
    return 0;
}

void svm_run_guest(struct SvmState *state)
{
    struct Vmcb *v = state->ss_vmcb;
    uint64_t vmcb_pa = (uint64_t)v;   /* identity mapped: VA == PA */

    /* Copy the guest boot stub to physical address 0x8000.
     * Source: sys/core/boot_stub.asm (assembled to boot_stub.h by make). */
    #include "boot_stub.h"
    {
        volatile uint8_t *dst = (volatile uint8_t *)0x8000;
        const uint8_t    *src = boot_stub_bytes;
        unsigned int      i;
        for (i = 0; i < boot_stub_bytes_len; i++)
            dst[i] = src[i];
    }

    hv_printf(&debug_serial, "SVM: booting guest OS from HDD via INT 13h\n");
    hv_printf(display, "Booting guest OS...\n");
    hv_printf(display, "Launching guest...\n");

    while (1) {
        svm_vmrun(vmcb_pa, &state->ss_guest_regs);
        /* Guest RAX is auto-saved to VMCB.state.rax by hardware on VMEXIT;
         * the CPU register holds the restored host RAX (vmcb_pa) instead. */
        state->ss_guest_regs.rax = v->state.rax;
        svm_exit_dispatch(v, &state->ss_guest_regs, &state->ss_exit_count);
    }
}

/* HvOperations backend for AMD SVM */

static int  svm_ops_enable(void)     { return svm_enable(&svm_state); }
static void svm_ops_print_info(void) { svm_print_info(); }
static void svm_ops_run_guest(void)  { svm_run_guest(&svm_state); }

const struct HvOperations svm_ops = {
    .name          = "AMD SVM",
    .check_support = svm_check_support,
    .print_info    = svm_ops_print_info,
    .enable        = svm_ops_enable,
    .run_guest     = svm_ops_run_guest,
};
