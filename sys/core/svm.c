/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | AMD SVM: enable, VMCB init, guest run loop, exit handler   |
 * +------------------------------------------------------------+
 */
#include <cpu.h>
#include <hv_ops.h>
#include <interrupt.h>
#include <memory.h>
#include <print.h>
#include <string.h>
#include <svm.h>
#include <system.h>

extern struct CharacterDisplay  debug_serial;
extern struct CharacterDisplay *display;

struct SvmState svm_state;

/* "Flight recorder": ring buffer of the last exits, kept in BSS so it
 * survives a silent machine reset (RAM is not cleared on reset, proven
 * earlier by prev_code persisting). Dumped at the top of the next boot
 * to reveal exactly what the guest/hypervisor were doing right before
 * an uninterceptable reset occurred. */
#define TRACE_LEN 24
static struct
{
    uint64_t code;
    uint64_t rip;
    uint64_t info1;
    uint64_t seq;
} trace_buf[TRACE_LEN];
static uint32_t trace_idx   = 0;
static uint32_t trace_magic = 0;
#define TRACE_MAGIC 0x600DF00DU

static void trace_record(uint64_t code, uint64_t rip, uint64_t info1, uint64_t seq)
{
    /* trace_idx persists in BSS across silent resets by design (see comment
     * above trace_buf), but stale RAM from an earlier/incompatible build can
     * leave it holding a wildly out-of-range value. trace_dump() already
     * distrusts persisted trace_buf contents unless trace_magic == TRACE_MAGIC
     * — extend the same distrust to trace_idx before using it as an index,
     * otherwise this first write lands far outside trace_buf and faults the
     * host (observed: EXCEPTION 14 at this very function, err=0xA). */
    if (trace_magic != TRACE_MAGIC || trace_idx >= TRACE_LEN)
        trace_idx = 0;

    trace_buf[trace_idx].code  = code;
    trace_buf[trace_idx].rip   = rip;
    trace_buf[trace_idx].info1 = info1;
    trace_buf[trace_idx].seq   = seq;
    trace_idx                  = (trace_idx + 1) % TRACE_LEN;
    trace_magic                = TRACE_MAGIC;
}

static void trace_dump(void)
{
    if (trace_magic != TRACE_MAGIC)
        return; /* first boot — nothing recorded yet */

    hv_printf(&debug_serial, "--- flight recorder: last %d exits before reset ---\n", TRACE_LEN);
    uint32_t i;
    for (i = 0; i < TRACE_LEN; i++) {
        uint32_t idx = (trace_idx + i) % TRACE_LEN;
        hv_printf(&debug_serial, "  #%u code=%x rip=%x info1=%x\n",
                  (unsigned)trace_buf[idx].seq,
                  (unsigned)trace_buf[idx].code,
                  (unsigned)trace_buf[idx].rip,
                  (unsigned)trace_buf[idx].info1);
    }
    hv_printf(&debug_serial, "--- end flight recorder ---\n");
    trace_magic = 0; /* consume — don't redump on the boot after this one */
}

/* 4KB host save area — written to MSR_VM_HSAVE_PA */
static uint8_t svm_host_save[PAGE_SIZE_4K] __aligned_4k;

/* Static VMCB (4KB aligned) */
static struct Vmcb svm_vmcb __aligned_4k;

/* I/O Permission Map (12KB, AMD APM §15.10.1) and MSR Permission Map (8KB).
 * Kept in HypoV's own BSS so guest writes to low memory can never clear the
 * reset-port intercept bits (the old IVT-alias trick failed once syslinux
 * reprogrammed its own interrupt vectors). */
static uint8_t svm_iopm[3 * PAGE_SIZE_4K] __aligned_4k;
static uint8_t svm_msrpm[2 * PAGE_SIZE_4K] __aligned_4k;

/* Set the intercept bit for a single I/O port in svm_iopm. */
static inline void iopm_set(uint16_t port)
{
    svm_iopm[port / 8] |= (uint8_t)(1u << (port % 8));
}

/* -----------------------------------------------------------------------
 * VMCB initialisation
 * ----------------------------------------------------------------------- */

static void vmcb_set_segment(struct SvmSegment *s, uint16_t sel, uint64_t base,
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
    vmcb_set_segment(&ss->cs, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_CODE);
    vmcb_set_segment(&ss->ds, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    vmcb_set_segment(&ss->ss, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    vmcb_set_segment(&ss->es, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    vmcb_set_segment(&ss->fs, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    vmcb_set_segment(&ss->gs, 0, 0, 0xFFFF, SVM_ATTRIB_REALMODE_DATA);
    vmcb_set_segment(&ss->ldtr, 0, 0, 0xFFFF, SVM_ATTRIB_LDTR);
    vmcb_set_segment(&ss->tr, 0, 0, 0xFFFF, SVM_ATTRIB_TR);

    ss->gdtr.base  = 0;
    ss->gdtr.limit = 0xFFFF;
    ss->idtr.base  = 0;
    ss->idtr.limit = 0x03FF; /* real-mode IVT */

    ss->efer   = EFER_SVME; /* SVME required by AMD APM and QEMU */
    ss->cr0    = CR0_ET;
    ss->cr3    = 0;
    ss->cr4    = 0;
    ss->dr6    = 0xFFFF0FF0; /* reserved bits must be 1 (AMD APM) */
    ss->dr7    = 0x400;
    ss->rflags = 0x2;    /* reserved bit always 1 */
    ss->rip    = 0x8000; /* HDD boot stub */
    ss->rsp    = 0x7C00;
    ss->rax    = 0;
    ss->cpl    = 0;
}

static void vmcb_init_controls(struct Vmcb *v, uint64_t npt_root)
{
    struct SvmControl *c = &v->control;

    c->guest_asid = 1; /* ASID 0 reserved for host */

    c->icept_misc1 = SVM_ICEPT_INTR |
                     SVM_ICEPT_INIT |
                     SVM_ICEPT_CPUID |
                     SVM_ICEPT_RDTSC |
                     SVM_ICEPT_HLT |
                     SVM_ICEPT_IOIO |
                     SVM_ICEPT_SHUTDOWN;

    c->icept_misc2 = SVM_ICEPT_VMRUN | /* always intercept nested VMRUN */
                     SVM_ICEPT_VMMCALL |
                     SVM_ICEPT_RDTSCP;

    /* Intercept PIC ports so IRQ0 masking can be enforced, and the three
     * hardware-reset ports so a panicking guest cannot drive QEMU to reset. */
    iopm_set(PIC0_CMD_PORT);
    iopm_set(PIC0_DATA_PORT);
    iopm_set(PIC1_CMD_PORT);
    iopm_set(PIC1_DATA_PORT);
    iopm_set(KBC_DATA_PORT);
    iopm_set(KBC_CMD_PORT);
    iopm_set(FAST_A20_PORT);
    iopm_set(PCI_RESET_PORT);

    c->iopm_base_pa  = (uint64_t)svm_iopm;
    c->msrpm_base_pa = (uint64_t)svm_msrpm; /* all zero → no MSR intercepts */

    c->np_enable = 1;
    c->n_cr3     = npt_root;
}

/* -----------------------------------------------------------------------
 * Helpers used by the exit dispatcher
 * ----------------------------------------------------------------------- */

static void svm_skip_instruction(struct Vmcb *v)
{
    /* nrip_save feature: holds the next sequential RIP, when supported */
    if (v->control.n_rip) {
        v->state.rip = v->control.n_rip;
        return;
    }
    /* IOIO exits: exit_info2 holds the next RIP */
    if (v->control.exit_code == SVM_EXIT_IOIO) {
        v->state.rip = v->control.exit_info2;
        return;
    }
    /* Fallback: known instruction lengths for QEMU TCG (no nrip_save) */
    uint32_t len;
    switch (v->control.exit_code) {
    case SVM_EXIT_CPUID:
        len = 2;
        break;
    case SVM_EXIT_RDTSC:
        len = 2;
        break;
    case SVM_EXIT_RDTSCP:
        len = 3;
        break;
    case SVM_EXIT_HLT:
        len = 1;
        break;
    case SVM_EXIT_VMMCALL:
        len = 3;
        break;
    case SVM_EXIT_RDPMC:
        len = 2;
        break;
    default:
        len = 1;
        break;
    }
    v->state.rip += len;
}

/* -----------------------------------------------------------------------
 * Per-exit-type handlers
 * ----------------------------------------------------------------------- */

static void handle_svm_cpuid(struct Vmcb *v, struct GuestRegisters *regs)
{
    int32_t  out[4];
    uint32_t leaf    = (uint32_t)regs->rax;
    uint32_t subleaf = (uint32_t)regs->rcx;

    __asm__ __volatile__("cpuid"
                         : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
                         : "a"(leaf), "c"(subleaf));

    switch (leaf) {
    case 0x1:
        out[2] &= ~CPUID_FEATURE_HYPERVISOR; /* hide hypervisor presence from guest */
        break;
    case 0x80000001:
        out[2] &= ~CPUID_EXT_FEATURE_SVM; /* hide SVM support from guest */
        break;
    case 0x40000000:
        out[0] = out[1] = out[2] = out[3] = 0;
        break;
    }

    regs->rax    = (uint64_t)(uint32_t)out[0];
    regs->rbx    = (uint64_t)(uint32_t)out[1];
    regs->rcx    = (uint64_t)(uint32_t)out[2];
    regs->rdx    = (uint64_t)(uint32_t)out[3];
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
        hv_printf(&debug_serial, "VMMCALL: HypoV hypercall OK exit_count=%d\n",
                  *exit_count);
        hv_printf(display, "VMMCALL: HypoV hypercall OK exit_count=%d\n",
                  *exit_count);
    } else {
        regs->rbx = regs->rcx = 0;
    }
    v->state.rax = regs->rax;
    svm_skip_instruction(v);
}

/* KBC ("Write Output Port") and reset-line command/bit values used to
 * recognize a guest trying to drive a hardware reset through the legacy
 * ports — see the comment in handle_svm_io for the full detection logic. */
#define KBC_CMD_WRITE_OUTPUT_PORT 0xD1     /* next byte to KBC_DATA_PORT drives the Output Port lines */
#define KBC_CMD_SYSTEM_RESET      0xFE     /* dedicated "pulse CPU reset" command */
#define KBC_CMD_PULSE_MASK        0xF0     /* "Pulse Output Port" range 0xF0..0xFF (classic reset/A20 trick) */
#define KBC_OUTPUT_RESET_BIT      (1 << 0) /* Output Port bit 0: 0 = CPU reset asserted */
#define FAST_A20_RESET_BIT        (1 << 0) /* port 0x92 bit 0: 1 = pulse CPU reset (INIT#) */
#define PCI_RESET_TRIGGER_BIT     (1 << 2) /* port 0xCF9 bit 2: 1 = trigger reset */

static void handle_svm_io(struct Vmcb *v, struct GuestRegisters *regs)
{
    uint64_t info  = v->control.exit_info1;
    uint16_t port  = (uint16_t)(info >> SVM_IOIO_PORT_SHIFT);
    int      is_in = (int)(info & SVM_IOIO_TYPE_IN);

    if (is_in) {
        /* IN only updates the bits matching the operand size — al/ax stay
         * within the existing eax, while a 32-bit read zero-extends to rax. */
        if (info & SVM_IOIO_SZ8) {
            uint8_t val = inb(port);
            regs->rax   = (regs->rax & ~(uint64_t)0xFF) | val;
        } else if (info & SVM_IOIO_SZ16) {
            uint16_t val = inw(port);
            regs->rax    = (regs->rax & ~(uint64_t)0xFFFF) | val;
        } else if (info & SVM_IOIO_SZ32) {
            regs->rax = inl(port);
        }
        v->state.rax = regs->rax;
    } else {
        uint8_t    val8                  = (uint8_t)regs->rax;
        static int kbc_write_output_port = 0;

        /* Swallow hardware reset commands: 0xCF9 (ACPI), 0x92 (fast A20),
         * 0x64<-0xFE (KBC dedicated System Reset), 0x64<-0xF0..0xFF (KBC
         * "Pulse Output Port" — the classic reset/A20 trick; no legitimate
         * OS uses this range for anything else), and the two-step KBC
         * "Write Output Port" sequence (0x64<-0xD1 then 0x60<-val with
         * bit0=0 pulses CPU reset).
         * Without this a panicking/rebooting guest resets all of QEMU. */
        if (port == KBC_CMD_PORT && val8 == KBC_CMD_WRITE_OUTPUT_PORT) {
            kbc_write_output_port = 1;
        } else if (port == KBC_DATA_PORT && kbc_write_output_port) {
            kbc_write_output_port = 0;
            if (!(val8 & KBC_OUTPUT_RESET_BIT)) {
                hv_printf(&debug_serial, "RESET swallowed port=%x val=%x rip=%x (KBC write-output-port)\n",
                          port, val8, (unsigned)v->state.rip);
                svm_skip_instruction(v);
                return;
            }
        } else if (port == KBC_CMD_PORT || port == KBC_DATA_PORT) {
            kbc_write_output_port = 0;
        }

        if ((port == PCI_RESET_PORT && (val8 & PCI_RESET_TRIGGER_BIT)) ||
            (port == KBC_CMD_PORT && val8 == KBC_CMD_SYSTEM_RESET) ||
            (port == KBC_CMD_PORT && (val8 & KBC_CMD_PULSE_MASK) == KBC_CMD_PULSE_MASK) ||
            (port == FAST_A20_PORT && (val8 & FAST_A20_RESET_BIT))) {
            hv_printf(&debug_serial, "RESET swallowed port=%x val=%x rip=%x\n",
                      port, val8, (unsigned)v->state.rip);
            svm_skip_instruction(v);
            return;
        }
        if (info & SVM_IOIO_SZ8)
            outb(port, val8);
        else if (info & SVM_IOIO_SZ16)
            outw(port, (uint16_t)regs->rax);
        else if (info & SVM_IOIO_SZ32)
            outl(port, (uint32_t)regs->rax);
    }
    svm_skip_instruction(v);
}

static void handle_svm_intr(struct Vmcb *v)
{
    /* Clear V_IRQ before VMRUN to avoid immediately re-exiting. */
    v->control.vintr &= ~(1ULL << 8);

    /* OCW3 poll (P=1): moves the highest-priority IRQ from IRR→ISR.
     * Follow with EOI so the PIC is ready for the next tick. */
    outb(0x20, 0x0C);
    uint8_t poll = inb(0x20);
    outb(0x20, 0x20); /* master EOI */

    if (!(poll & 0x80))
        return; /* spurious */

    int     irq   = poll & 0x07;
    int     in_pm = (int)(v->state.cr0 & 1);
    uint8_t vector;

    if (irq == 2) {
        /* Cascade: poll slave PIC */
        outb(0xA0, 0x0C);
        uint8_t poll2 = inb(0xA0);
        outb(0xA0, 0x20); /* slave EOI */
        if (!(poll2 & 0x80))
            return;
        vector = (in_pm ? 0x28 : 0x70) + (poll2 & 0x07);
    } else {
        vector = (in_pm ? 0x20 : 0x08) + irq;
    }

    if (!in_pm) {
        /* Real mode: inject unconditionally so BIOS INT 8 increments 0x46C */
        v->control.event_inject = SVM_EVENTINJ_VALID | vector;
        return;
    }

    v->control.event_inject = SVM_EVENTINJ_VALID | vector;

    if (v->state.efer & EFER_LMA) {
        static int lm_first = 0;
        if (!lm_first) {
            hv_printf(&debug_serial, "TIMER_LM: first inject irq%d vec=%x rip=%x\n",
                      irq, vector, (unsigned)v->state.rip);
            lm_first = 1;
        }
    }
}

/* -----------------------------------------------------------------------
 * Main exit dispatcher
 * ----------------------------------------------------------------------- */

#ifdef CONFIG_HV_PHASE_LOG
/* One-line trace on each exit-type transition to show boot phases. */
static void svm_log_phase(struct Vmcb *v, uint64_t code)
{
    static uint64_t prev_code = 0;

    if (code != prev_code) {
        hv_printf(&debug_serial,
                  "PHASE %x->%x cr0=%x efer=%x rip=%x\n",
                  (unsigned)prev_code, (unsigned)code,
                  (unsigned)v->state.cr0, (unsigned)v->state.efer,
                  (unsigned)v->state.rip);
        prev_code = code;
    }
}
#else
static inline void svm_log_phase(struct Vmcb *v, uint64_t code) {}
#endif

static void svm_exit_dispatch(struct Vmcb *v, struct GuestRegisters *regs,
                              uint64_t *exit_count)
{
    uint64_t code = v->control.exit_code;

    (*exit_count)++;

    trace_record(code, v->state.rip, v->control.exit_info1, *exit_count);

    svm_log_phase(v, code);

    switch (code) {
    case SVM_EXIT_INTR:
        handle_svm_intr(v);
        break;
    case SVM_EXIT_INIT:
        /* A guest-issued INIT (e.g. APIC self-IPI during SMP probe) would,
         * if left unintercepted, reset the physical CPU and take the whole
         * host down with it.  Emulate it instead: restart the guest from
         * its boot entry, exactly like a real CPU INIT restarts execution
         * from the reset vector — but without disturbing the hypervisor. */
        hv_printf(&debug_serial, "INIT intercepted RIP=%x — restarting guest\n",
                  (unsigned)v->state.rip);
        vmcb_init_guest_state(v);
        return;
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
    case SVM_EXIT_MSR: {
        /* MSR intercept: pass through, but guard EFER so a guest write
         * doesn't corrupt the host's own 64-bit mode. */
        uint32_t msr = (uint32_t)regs->rcx;
        if (v->control.exit_info1 == 0) { /* RDMSR */
            uint64_t val = (msr == MSR_IA32_EFER) ? v->state.efer
                                                  : msr_read(msr);
            regs->rax    = (uint32_t)val;
            regs->rdx    = (uint32_t)(val >> 32);
            v->state.rax = regs->rax;
        } else { /* WRMSR */
            uint64_t val = ((uint64_t)(uint32_t)regs->rdx << 32) | (uint32_t)regs->rax;
            if (msr == MSR_IA32_EFER)
                v->state.efer = val;
            else
                msr_write(msr, val);
        }
        v->state.rip += 2; /* RDMSR/WRMSR are 2-byte opcodes */
        break;
    }
    case SVM_EXIT_NPF:
        hv_printf(&debug_serial, "NPF GPA=%x RIP=%x\n",
                  (unsigned)v->control.exit_info2, (unsigned)v->state.rip);
        hv_printf(display, "NPF GPA=%x\n", (unsigned)v->control.exit_info2);
        goto halt;
    case SVM_EXIT_SHUTDOWN:
        hv_printf(&debug_serial, "Guest shutdown RIP=%x CR0=%x\n",
                  (unsigned)v->state.rip, (unsigned)v->state.cr0);
        hv_printf(display, "Guest shutdown RIP=%x\n", (unsigned)v->state.rip);
        goto halt;
    case SVM_EXIT_INVALID:
        hv_printf(&debug_serial, "SVM_EXIT_INVALID RIP=%x CR0=%x\n",
                  (unsigned)v->state.rip, (unsigned)v->state.cr0);
        goto halt;
    default:
        hv_printf(&debug_serial, "Unhandled exit code=%x RIP=%x\n",
                  (unsigned)code, (unsigned)v->state.rip);
        svm_skip_instruction(v);
        break;
    }

    if (v->state.efer & EFER_LMA) {
        /* Long mode: the kernel uses the LAPIC for timer, not the PIC.
         * Disable INTR intercept so LAPIC interrupts pass to the guest
         * without exiting.  Linux's LM idle is "sti; hlt" (IF=1), so
         * the LAPIC timer wakes it up naturally. */
        v->control.icept_misc1 &= ~SVM_ICEPT_INTR;
        v->control.vintr = 0;
    } else if (code != SVM_EXIT_INTR && (v->state.cr0 & 1)) {
        /* Protected mode (pre-LMA): PIC-based PIT delivery.
         * Arm V_INTR so the next STI causes an immediate INTR exit and
         * we can inject the tick.  Without this, cli;hlt loops freeze. */
        outb(0x20, 0x0A); /* read PIC IRR */
        if (inb(0x20) & 0x01) {
            v->control.vintr = (1ULL << 8) |           /* V_IRQ */
                               (0xFULL << 12) |        /* V_INTR_PRIO=15 */
                               (1ULL << 16) |          /* V_IGN_TPR */
                               ((uint64_t)0x20 << 32); /* V_INTR_VECTOR=0x20 */
        }
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
    if (!(regs[CPUID_REG_ECX] & CPUID_EXT_FEATURE_SVM)) {
        hv_printf(&debug_serial, "SVM: not supported\n");
        hv_printf(display, "SVM: not supported\n");
        return -1;
    }
    if (msr_read(MSR_VM_CR) & VM_CR_SVMDIS) {
        hv_printf(&debug_serial, "SVM: disabled in BIOS\n");
        hv_printf(display, "SVM: disabled in BIOS\n");
        return -1;
    }
    return 0;
}

void svm_print_info(void)
{
    hv_printf(display, "SVM: supported\n");
    hv_printf(display, "NPT: %dMB mapped\n", 512);
    hv_printf(&debug_serial, "SVM: VMCB at %x, host_save at %x\n",
              &svm_vmcb, svm_host_save);
}

int svm_enable(struct SvmState *state)
{
    msr_write(MSR_IA32_EFER, msr_read(MSR_IA32_EFER) | EFER_SVME);
    msr_write(MSR_VM_HSAVE_PA, (uint64_t)svm_host_save);

    uint64_t npt_root = npt_build();

    bzero(&svm_vmcb, sizeof(svm_vmcb));
    vmcb_init_controls(&svm_vmcb, npt_root);
    vmcb_init_guest_state(&svm_vmcb);

    state->ss_vmcb       = &svm_vmcb;
    state->ss_exit_count = 0;
    bzero(&state->ss_guest_regs, sizeof(state->ss_guest_regs));

    hv_printf(&debug_serial, "SVM: active\n");
    hv_printf(display, "SVM: active\n");
    return 0;
}

void svm_run_guest(struct SvmState *state)
{
    struct Vmcb *v       = state->ss_vmcb;
    uint64_t     vmcb_pa = (uint64_t)v; /* identity mapped: VA == PA */

    trace_dump();

/* Copy the guest boot stub to 0x8000 (loads HDD MBR via INT 13h). */
#include "boot_stub.h"
    {
        volatile uint8_t *dst = (volatile uint8_t *)0x8000;
        const uint8_t    *src = boot_stub_bytes;
        unsigned int      i;
        for (i = 0; i < boot_stub_bytes_len; i++)
            dst[i] = src[i];
    }

    hv_printf(&debug_serial, "SVM: booting guest via INT 13h\n");
    hv_printf(display, "Launching guest...\n");

    while (1) {
        v->control.exit_code = SVM_EXIT_INVALID;
        svm_vmrun(vmcb_pa, &state->ss_guest_regs);
        state->ss_guest_regs.rax = v->state.rax;
        svm_exit_dispatch(v, &state->ss_guest_regs, &state->ss_exit_count);
    }
}

/* HvOperations backend for AMD SVM */

static int  svm_ops_enable(void) { return svm_enable(&svm_state); }
static void svm_ops_print_info(void) { svm_print_info(); }
static void svm_ops_run_guest(void) { svm_run_guest(&svm_state); }

struct HvOperations svm_ops;

void svm_backend_init(struct HvOperations *ops)
{
    ops->name          = "AMD SVM";
    ops->check_support = svm_check_support;
    ops->print_info    = svm_ops_print_info;
    ops->enable        = svm_ops_enable;
    ops->run_guest     = svm_ops_run_guest;
}
