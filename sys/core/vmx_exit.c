/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2024                           |
 * |                                                            |
 * | VM exit handler — dispatches on exit reason, handles       |
 * | CPUID masking, RDTSC, VMCALL hypercall, and I/O passthrough|
 * +------------------------------------------------------------+
*/
#include <system.h>
#include <cpu.h>
#include <print.h>
#include <vmx.h>

extern struct CharacterDisplay debug_serial;
extern struct CharacterDisplay *display;
extern struct VmxState vmx_state;

static void vmx_skip_instruction(void)
{
    uint64_t rip = vmx_read(VMCS_GUEST_RIP);
    uint64_t len = vmx_read(VMCS_VM_EXIT_INSTR_LEN);
    vmx_write(VMCS_GUEST_RIP, rip + len);
}

static void vmx_cpuid_ex(uint32_t leaf, uint32_t subleaf, int32_t out[4])
{
    __asm__ __volatile__(
        "cpuid"
        : "=a"(out[0]), "=b"(out[1]), "=c"(out[2]), "=d"(out[3])
        : "a"(leaf), "c"(subleaf)
    );
}

static void handle_cpuid(struct GuestRegisters *regs)
{
    int32_t out[4];
    uint32_t leaf    = (uint32_t)regs->rax;
    uint32_t subleaf = (uint32_t)regs->rcx;

    vmx_cpuid_ex(leaf, subleaf, out);

    switch (leaf) {
    case 0x1:
        out[2] &= ~(1 << 31); /* clear hypervisor-present bit */
        break;
    case 0x40000000:
        out[0] = out[1] = out[2] = out[3] = 0; /* no hypervisor vendor */
        break;
    }

    regs->rax = (uint64_t)(uint32_t)out[0];
    regs->rbx = (uint64_t)(uint32_t)out[1];
    regs->rcx = (uint64_t)(uint32_t)out[2];
    regs->rdx = (uint64_t)(uint32_t)out[3];
    vmx_skip_instruction();
}

static void handle_rdtsc(struct GuestRegisters *regs)
{
    uint32_t lo, hi;
    rdtsc(&hi, &lo);
    regs->rax = lo;
    regs->rdx = hi;
    vmx_skip_instruction();
}

static void handle_vmcall(struct GuestRegisters *regs)
{
    if (regs->rax == HV_HYPERCALL_SIGNATURE) {
        regs->rbx = HV_HYPERCALL_MAGIC;
        regs->rcx = vmx_state.vs_exit_count;
        hv_printf(&debug_serial, "VMCALL: HypoV hypercall OK! magic=%x exit_count=%d\n",
                  HV_HYPERCALL_MAGIC, vmx_state.vs_exit_count);
        hv_printf(display, "VMCALL: guest confirmed running under HypoV!\n");
    } else {
        regs->rbx = 0;
        regs->rcx = 0;
    }
    vmx_skip_instruction();
}

static void handle_io(struct GuestRegisters *regs)
{
    uint64_t qual  = vmx_read(VMCS_EXIT_QUALIFICATION);
    uint16_t port  = (uint16_t)(qual >> 16);
    int      is_in = (int)((qual >> 3) & 1);
    int      size  = (int)(qual & 7); /* 0=byte, 1=word, 3=dword */

    if (is_in) {
        switch (size) {
        case 0: regs->rax = (regs->rax & ~0xFFULL)         | inb(port); break;
        case 1: regs->rax = (regs->rax & ~0xFFFFULL)       | inw(port); break;
        case 3: regs->rax = (regs->rax & ~0xFFFFFFFFULL)   | inl(port); break;
        default: regs->rax = 0xFFFFFFFF; break;
        }
    } else {
        switch (size) {
        case 0: outb(port, (uint8_t)regs->rax);  break;
        case 1: outw(port, (uint16_t)regs->rax); break;
        case 3: outl(port, (uint32_t)regs->rax); break;
        default: break;
        }
    }
    vmx_skip_instruction();
}

static void handle_rdmsr(struct GuestRegisters *regs)
{
    uint32_t msr = (uint32_t)regs->rcx;
    uint32_t lo, hi;

    /* EFER: return VMCS guest EFER so the guest sees correct state */
    if (msr == MSR_IA32_EFER) {
        uint64_t guest_efer = vmx_read(VMCS_GUEST_IA32_EFER);
        lo = (uint32_t)(guest_efer & 0xFFFFFFFF);
        hi = (uint32_t)(guest_efer >> 32);
    } else if (msr >= 0x480 && msr <= 0x493) {
        /* VMX MSRs: hide from guest */
        lo = hi = 0;
    } else {
        /* Passthrough: all other MSR reads go to hardware */
        __asm__ __volatile__("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    }
    regs->rax = lo;
    regs->rdx = hi;
    vmx_skip_instruction();
}

static void handle_wrmsr(struct GuestRegisters *regs)
{
    uint32_t msr = (uint32_t)regs->rcx;
    uint32_t lo  = (uint32_t)regs->rax;
    uint32_t hi  = (uint32_t)regs->rdx;

    /* Block writes to hypervisor-sensitive MSRs */
    if (msr == MSR_IA32_EFER ||
        (msr >= 0x480 && msr <= 0x493) ||
        (msr >= 0xC0010000 && msr <= 0xC0011FFF))
        goto skip;

    /* Passthrough: write to hardware */
    __asm__ __volatile__("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
skip:
    vmx_skip_instruction();
}

void vmx_exit_handler(struct GuestRegisters *regs)
{
    uint32_t reason = (uint32_t)vmx_read(VMCS_VM_EXIT_REASON) & 0xFFFF;

    vmx_state.vs_exit_count++;

    switch (reason) {
    case EXIT_REASON_EXTERNAL_INTERRUPT:
        /* Interrupt handled by host IDT during exit; just resume */
        return;
    case EXIT_REASON_CPUID:
        handle_cpuid(regs);
        break;
    case EXIT_REASON_RDTSC:
    case EXIT_REASON_RDTSCP:
        handle_rdtsc(regs);
        break;
    case EXIT_REASON_VMCALL:
        handle_vmcall(regs);
        break;
    case EXIT_REASON_IO_INSTRUCTION:
        handle_io(regs);
        break;
    case EXIT_REASON_HLT:
        vmx_skip_instruction();
        break;
    case EXIT_REASON_RDMSR:
        handle_rdmsr(regs);
        break;
    case EXIT_REASON_WRMSR:
        handle_wrmsr(regs);
        break;
    case EXIT_REASON_EPT_VIOLATION:
    case EXIT_REASON_EPT_MISCONFIG: {
        uint64_t gpa = vmx_read(VMCS_GUEST_PHYS_ADDR);
        uint64_t gip = vmx_read(VMCS_GUEST_RIP);
        uint64_t qual = vmx_read(VMCS_EXIT_QUALIFICATION);
        hv_printf(&debug_serial, "EPT violation: GPA=%x GIP=%x qual=%x %s%s%s\n",
                  gpa, gip, qual,
                  (qual & 1) ? "R" : "", (qual & 2) ? "W" : "", (qual & 4) ? "X" : "");
        hv_printf(display, "EPT violation GPA=%x\n", gpa);
        goto halt;
    }
    case EXIT_REASON_TRIPLE_FAULT: {
        uint64_t rip = vmx_read(VMCS_GUEST_RIP);
        uint64_t cr0 = vmx_read(VMCS_GUEST_CR0);
        hv_printf(&debug_serial, "Triple fault: RIP=%x CR0=%x\n", rip, cr0);
        hv_printf(display, "Guest triple fault RIP=%x\n", rip);
        goto halt;
    }
    case EXIT_REASON_INVALID_GUEST:
        hv_printf(&debug_serial, "Invalid guest state, error=%d\n",
                  (uint32_t)vmx_read(VMCS_VM_INSTR_ERROR));
        hv_printf(display, "Invalid guest state\n");
        goto halt;
    default:
        hv_printf(&debug_serial, "Unhandled exit: reason=%d RIP=%x\n",
                  reason, vmx_read(VMCS_GUEST_RIP));
        vmx_skip_instruction();
        break;
    }
    return;
halt:
    while (1)
        ;
}
