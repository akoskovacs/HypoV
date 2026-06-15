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
    int      size  = (int)(qual & 7);

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
    uint64_t val = regs->rax | ((uint64_t)regs->rdx << 32);

    switch (msr) {
    case MSR_IA32_EFER:
        /* Update guest EFER in VMCS so the guest can enable long mode */
        vmx_write(VMCS_GUEST_IA32_EFER, val);
        break;
    case MSR_GS_BASE:
        vmx_write(VMCS_GUEST_GS_BASE, val);
        break;
    case MSR_FS_BASE:
        vmx_write(VMCS_GUEST_FS_BASE, val);
        break;
    case MSR_KERNEL_GS_BASE:
        /*
         * No VMCS guest field — write to hardware.  The value persists
         * across VM exits/entries and SWAPGS will swap GS_BASE with it
         * directly.  This DOES corrupt L1's MSR_KERNEL_GS_BASE, but
         * fixing that requires guest/host MSR load lists.
         */
        __asm__ __volatile__("wrmsr" : : "a"((uint32_t)val),
                             "d"((uint32_t)(val >> 32)), "c"(msr));
        break;
    default:
        if ((msr >= 0x480 && msr <= 0x493) ||
            (msr >= 0xC0010000 && msr <= 0xC0011FFF)) {
            /* VMX MSRs: silently ignore */
        } else {
            /* Passthrough: write to hardware */
            __asm__ __volatile__("wrmsr" : : "a"((uint32_t)regs->rax),
                                 "d"((uint32_t)regs->rdx), "c"(msr));
        }
        break;
    }
    vmx_skip_instruction();
}

static uint64_t guest_page_walk(uint64_t cr3, uint64_t va)
{
    hv_printf(&debug_serial, "  Walk: CR3=%x VA=%x\n", cr3, va);
    uint64_t pml4e = *(volatile uint64_t *)(unsigned long)(cr3 + ((va >> 39) & 0x1FF) * 8);
    hv_printf(&debug_serial, "    PML4[%u]=%x\n", (unsigned)((va >> 39) & 0x1FF), pml4e);
    if (!(pml4e & PML_PRESENT))
        return 0;

    uint64_t pdpte = *(volatile uint64_t *)(unsigned long)((pml4e & PML_BASE_MASK) + ((va >> 30) & 0x1FF) * 8);
    hv_printf(&debug_serial, "    PDPT[%u]=%x\n", (unsigned)((va >> 30) & 0x1FF), pdpte);
    if (!(pdpte & PML_PRESENT))
        return 0;
    if (pdpte & PML2_PS) {
        uint64_t pa = (pdpte & 0xFFFFFC0000000ULL) | (va & 0x3FFFFFFF);
        hv_printf(&debug_serial, "    1G page -> PA=%x\n", pa);
        return pa;
    }

    uint64_t pde = *(volatile uint64_t *)(unsigned long)((pdpte & PML_BASE_MASK) + ((va >> 21) & 0x1FF) * 8);
    hv_printf(&debug_serial, "    PD[%u]=%x\n", (unsigned)((va >> 21) & 0x1FF), pde);
    if (!(pde & PML_PRESENT))
        return 0;
    if (pde & PML2_PS) {
        uint64_t pa = (pde & PAGE_MASK_2M) | (va & 0x1FFFFF);
        hv_printf(&debug_serial, "    2M page -> PA=%x\n", pa);
        return pa;
    }

    uint64_t pte = *(volatile uint64_t *)(unsigned long)((pde & PML_BASE_MASK) + ((va >> 12) & 0x1FF) * 8);
    hv_printf(&debug_serial, "    PT[%u]=%x\n", (unsigned)((va >> 12) & 0x1FF), pte);
    if (!(pte & PML_PRESENT))
        return 0;
    uint64_t pa = (pte & PML_BASE_MASK) | (va & 0xFFF);
    hv_printf(&debug_serial, "    4K page -> PA=%x\n", pa);
    return pa;
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
    case EXIT_REASON_MOV_CR: {
        uint64_t qual = vmx_read(VMCS_EXIT_QUALIFICATION);
        uint32_t cr  = qual & 0xF;
        uint32_t dir = (qual >> 4) & 1;
        /* dir=0: MOV CRx, reg — write to CR */
        if (dir == 0) {
            uint32_t src_reg = (qual >> 8) & 0xF;
            static const uint8_t reg_map[] = {
                [0]  = 0,  [1]  = 2,  [2]  = 3,  [3]  = 1,
                [5]  = 4,  [6]  = 5,  [7]  = 6,
                [8]  = 7,  [9]  = 8,  [10] = 9,
                [11] = 10, [12] = 11, [13] = 12,
                [14] = 13, [15] = 14,
            };
            uint64_t *r = ((uint64_t *)regs) + reg_map[src_reg];
            uint64_t val = *r;
            switch (cr) {
            case 3:
                vmx_write(VMCS_GUEST_CR3, val);
                break;
            case 4:
                hv_printf(&debug_serial, "CR4: wrote %x (from reg %u)\n",
                          (unsigned)val, src_reg);
                val |= CR4_VMXE;
                vmx_write(VMCS_GUEST_CR4, val);
                break;
            case 0:
                vmx_write(VMCS_GUEST_CR0, val);
                break;
            }
        }
        vmx_skip_instruction();
        break;
    }
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
        uint64_t rip   = vmx_read(VMCS_GUEST_RIP);
        uint64_t rsp   = vmx_read(VMCS_GUEST_RSP);
        uint64_t cr0   = vmx_read(VMCS_GUEST_CR0);
        uint64_t cr2   = cr2_read64();
        uint64_t cr3   = vmx_read(VMCS_GUEST_CR3);
        uint64_t cr4   = vmx_read(VMCS_GUEST_CR4);
        uint64_t efer  = vmx_read(VMCS_GUEST_IA32_EFER);
        uint64_t gsbase = vmx_read(VMCS_GUEST_GS_BASE);
        uint64_t cs    = vmx_read(VMCS_GUEST_CS);
        uint64_t csar  = vmx_read(VMCS_GUEST_CS_AR);
        uint64_t gdtr  = vmx_read(VMCS_GUEST_GDTR_BASE);
        uint64_t gdtl  = vmx_read(VMCS_GUEST_GDTR_LIMIT);
        uint64_t idtr  = vmx_read(VMCS_GUEST_IDTR_BASE);
        uint64_t idtl  = vmx_read(VMCS_GUEST_IDTR_LIMIT);
        uint64_t intr  = vmx_read(VMCS_VM_EXIT_INTR_INFO);
        uint64_t qual  = vmx_read(VMCS_EXIT_QUALIFICATION);
        hv_printf(&debug_serial, "Triple fault: RIP=%x RSP=%x CR0=%x CR2=%x CR3=%x CR4=%x EFER=%x\n",
                  rip, rsp, cr0, cr2, cr3, cr4, efer);
        hv_printf(&debug_serial, "  GS_BASE=%x FS_BASE=%x\n", gsbase, vmx_read(VMCS_GUEST_FS_BASE));
        hv_printf(&debug_serial, "  CS=%x AR=%x GDTR=%x/%x IDTR=%x/%x qual=%x\n",
                  cs, csar, gdtr, gdtl, idtr, idtl, qual);
        hv_printf(&debug_serial, "  RIP high=%x low=%x\n",
                  (unsigned)(rip >> 32), (unsigned)(rip & 0xFFFFFFFF));
        hv_printf(&debug_serial, "  RSP high=%x low=%x\n",
                  (unsigned)(rsp >> 32), (unsigned)(rsp & 0xFFFFFFFF));
        hv_printf(&debug_serial, "  Exit intr: vec=%u type=%u err=%u\n",
                  (unsigned)(intr & 0xFF), (unsigned)((intr >> 8) & 7), (unsigned)((intr >> 11) & 1));
        hv_printf(&debug_serial, "  Guest stack at %x (skipped)\n", rsp);
        uint64_t gpa = guest_page_walk(cr3, rip);
        if (gpa) {
            uint8_t *p = (uint8_t *)(unsigned long)gpa;
        hv_printf(&debug_serial, "  Instr at GPA=%x:", gpa);
        for (int i = 0; i < 16; i++)
            hv_printf(&debug_serial, " %x", (unsigned)p[i]);
        hv_printf(&debug_serial, "\n");
        } else {
            hv_printf(&debug_serial, "  Cannot translate RIP=%x via CR3=%x\n", rip, cr3);
        }
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
    /* Update IA32e guest bit in entry controls if guest is in long mode */
    {
        uint64_t efer = vmx_read(VMCS_GUEST_IA32_EFER);
        uint32_t entry = vmx_read(VMCS_VM_ENTRY_CTLS);
        if (efer & EFER_LMA) {
            if (!(entry & ENTRY_IA32E_GUEST)) {
                entry |= ENTRY_IA32E_GUEST;
                vmx_write(VMCS_VM_ENTRY_CTLS, entry);
            }
        } else {
            if (entry & ENTRY_IA32E_GUEST) {
                entry &= ~ENTRY_IA32E_GUEST;
                vmx_write(VMCS_VM_ENTRY_CTLS, entry);
            }
        }
    }
    return;
halt:
    while (1)
        ;
}
