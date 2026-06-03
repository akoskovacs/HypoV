/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2024                           |
 * |                                                            |
 * | Intel VT-x (VMX) definitions, VMCS field encodings,        |
 * | and data structures                                         |
 * +------------------------------------------------------------+
*/
#ifndef VMX_H
#define VMX_H

#include <basic.h>
#include <types.h>
#include <memory.h>

/* -----------------------------------------------------------------------
 * VMX capability MSRs
 * ----------------------------------------------------------------------- */
#define MSR_IA32_VMX_BASIC              0x480
#define MSR_IA32_VMX_PINBASED_CTLS     0x481
#define MSR_IA32_VMX_PROCBASED_CTLS    0x482
#define MSR_IA32_VMX_EXIT_CTLS         0x483
#define MSR_IA32_VMX_ENTRY_CTLS        0x484
#define MSR_IA32_VMX_MISC              0x485
#define MSR_IA32_VMX_CR0_FIXED0        0x486
#define MSR_IA32_VMX_CR0_FIXED1        0x487
#define MSR_IA32_VMX_CR4_FIXED0        0x488
#define MSR_IA32_VMX_CR4_FIXED1        0x489
#define MSR_IA32_VMX_VMCS_ENUM         0x48A
#define MSR_IA32_VMX_PROCBASED_CTLS2   0x48B
#define MSR_IA32_VMX_EPT_VPID_CAP     0x48C
#define MSR_IA32_VMX_TRUE_PINBASED     0x48D
#define MSR_IA32_VMX_TRUE_PROCBASED    0x48E
#define MSR_IA32_VMX_TRUE_EXIT         0x48F
#define MSR_IA32_VMX_TRUE_ENTRY        0x490

/* IA32_VMX_BASIC field masks */
#define VMX_BASIC_REVISION_MASK        0x7FFFFFFFUL
#define VMX_BASIC_VMCS_SIZE_SHIFT      32
#define VMX_BASIC_VMCS_SIZE_MASK       0x1FFFUL
#define VMX_BASIC_PHYS_64BIT           (1ULL << 48)
#define VMX_BASIC_MEMTYPE_SHIFT        50
#define VMX_BASIC_MEMTYPE_MASK         0xFULL
#define VMX_BASIC_MEMTYPE_WB           6ULL
#define VMX_BASIC_TRUE_CTLS            (1ULL << 55)  /* TRUE MSRs available */

/* -----------------------------------------------------------------------
 * Pin-based VM-execution control bits (VMCS_PIN_BASED_CTLS)
 * ----------------------------------------------------------------------- */
#define PIN_EXT_INT_EXITING            (1U << 0)
#define PIN_NMI_EXITING                (1U << 3)
#define PIN_VIRTUAL_NMIS               (1U << 5)
#define PIN_VMX_PREEMPT_TIMER          (1U << 6)
#define PIN_POSTED_INTERRUPTS          (1U << 7)

/* -----------------------------------------------------------------------
 * Primary processor-based VM-execution control bits (VMCS_CPU_BASED_CTLS)
 * ----------------------------------------------------------------------- */
#define PROC_INTR_WINDOW_EXITING       (1U << 2)
#define PROC_TSC_OFFSETTING            (1U << 3)
#define PROC_HLT_EXITING               (1U << 7)
#define PROC_INVLPG_EXITING            (1U << 9)
#define PROC_MWAIT_EXITING             (1U << 10)
#define PROC_RDPMC_EXITING             (1U << 11)
#define PROC_RDTSC_EXITING             (1U << 12)
#define PROC_CR3_LOAD_EXITING          (1U << 15)
#define PROC_CR3_STORE_EXITING         (1U << 16)
#define PROC_CR8_LOAD_EXITING          (1U << 19)
#define PROC_CR8_STORE_EXITING         (1U << 20)
#define PROC_USE_TPR_SHADOW            (1U << 21)
#define PROC_NMI_WINDOW_EXITING        (1U << 22)
#define PROC_MOV_DR_EXITING            (1U << 23)
#define PROC_UNCOND_IO_EXITING         (1U << 24)
#define PROC_USE_IO_BITMAPS            (1U << 25)
#define PROC_MONITOR_TRAP_FLAG         (1U << 27)
#define PROC_USE_MSR_BITMAPS           (1U << 28)
#define PROC_MONITOR_EXITING           (1U << 29)
#define PROC_PAUSE_EXITING             (1U << 30)
#define PROC_ACTIVATE_SECONDARY        (1U << 31)

/* -----------------------------------------------------------------------
 * Secondary processor-based VM-execution control bits (VMCS_CPU_BASED_CTLS2)
 * ----------------------------------------------------------------------- */
#define PROC2_VIRT_APIC                (1U << 0)
#define PROC2_EPT                      (1U << 1)
#define PROC2_DESCRIPTOR_TBL_EXITING   (1U << 2)
#define PROC2_RDTSCP                   (1U << 3)
#define PROC2_VIRT_X2APIC              (1U << 4)
#define PROC2_VPID                     (1U << 5)
#define PROC2_WBINVD_EXITING           (1U << 6)
#define PROC2_UNRESTRICTED_GUEST       (1U << 7)
#define PROC2_APIC_REG_VIRT            (1U << 8)
#define PROC2_VIRT_INTR_DELIVERY       (1U << 9)
#define PROC2_PAUSE_LOOP_EXITING       (1U << 10)
#define PROC2_RDRAND_EXITING           (1U << 11)
#define PROC2_INVPCID                  (1U << 12)
#define PROC2_VMFUNC                   (1U << 13)
#define PROC2_XSAVES_XRSTORS           (1U << 20)
#define PROC2_TSC_SCALING              (1U << 25)

/* -----------------------------------------------------------------------
 * VM-exit control bits (VMCS_VM_EXIT_CTLS)
 * ----------------------------------------------------------------------- */
#define EXIT_SAVE_DEBUG_CTLS           (1U << 2)
#define EXIT_HOST_64BIT                (1U << 9)   /* Host in 64-bit mode */
#define EXIT_ACK_INTR_ON_EXIT          (1U << 15)
#define EXIT_SAVE_PAT                  (1U << 18)
#define EXIT_LOAD_PAT                  (1U << 19)
#define EXIT_SAVE_EFER                 (1U << 20)
#define EXIT_LOAD_EFER                 (1U << 21)
#define EXIT_SAVE_PREEMPT_TIMER        (1U << 22)

/* -----------------------------------------------------------------------
 * VM-entry control bits (VMCS_VM_ENTRY_CTLS)
 * ----------------------------------------------------------------------- */
#define ENTRY_LOAD_DEBUG_CTLS          (1U << 2)
#define ENTRY_IA32E_GUEST              (1U << 9)   /* Guest in 64-bit long mode */
#define ENTRY_LOAD_PAT                 (1U << 14)
#define ENTRY_LOAD_EFER                (1U << 15)

/* -----------------------------------------------------------------------
 * VM exit reasons (VMCS_VM_EXIT_REASON low 16 bits)
 * ----------------------------------------------------------------------- */
#define EXIT_REASON_EXCEPTION_NMI      0
#define EXIT_REASON_EXTERNAL_INTERRUPT 1
#define EXIT_REASON_TRIPLE_FAULT       2
#define EXIT_REASON_INIT               3
#define EXIT_REASON_SIPI               4
#define EXIT_REASON_INTERRUPT_WINDOW   7
#define EXIT_REASON_NMI_WINDOW         8
#define EXIT_REASON_TASK_SWITCH        9
#define EXIT_REASON_CPUID              10
#define EXIT_REASON_HLT                12
#define EXIT_REASON_INVD               13
#define EXIT_REASON_RDPMC              15
#define EXIT_REASON_RDTSC              16
#define EXIT_REASON_VMCALL             18
#define EXIT_REASON_MOV_CR             28
#define EXIT_REASON_MOV_DR             29
#define EXIT_REASON_IO_INSTRUCTION     30
#define EXIT_REASON_RDMSR              31
#define EXIT_REASON_WRMSR              32
#define EXIT_REASON_INVALID_GUEST      33
#define EXIT_REASON_MWAIT              36
#define EXIT_REASON_MONITOR            39
#define EXIT_REASON_PAUSE              40
#define EXIT_REASON_MACHINE_CHECK      41
#define EXIT_REASON_EPT_VIOLATION      48
#define EXIT_REASON_EPT_MISCONFIG      49
#define EXIT_REASON_RDTSCP             51
#define EXIT_REASON_PREEMPT_TIMER      52
#define EXIT_REASON_XSETBV             55

/* -----------------------------------------------------------------------
 * VMCS field encodings
 * Encoding: bits[13:12]=width(00=16,01=64,10=32,11=natural)
 *           bits[11:10]=type(00=ctrl,01=exit-info,10=guest,11=host)
 *           bits[9:1]=index, bit[0]=access(0=full)
 * ----------------------------------------------------------------------- */

/* 16-bit control */
#define VMCS_VPID                      0x0000
#define VMCS_POSTED_INT_NV             0x0002
#define VMCS_EPTP_INDEX                0x0004

/* 16-bit guest state */
#define VMCS_GUEST_ES                  0x0800
#define VMCS_GUEST_CS                  0x0802
#define VMCS_GUEST_SS                  0x0804
#define VMCS_GUEST_DS                  0x0806
#define VMCS_GUEST_FS                  0x0808
#define VMCS_GUEST_GS                  0x080A
#define VMCS_GUEST_LDTR                0x080C
#define VMCS_GUEST_TR                  0x080E
#define VMCS_GUEST_INTR_STATUS         0x0810

/* 16-bit host state */
#define VMCS_HOST_ES                   0x0C00
#define VMCS_HOST_CS                   0x0C02
#define VMCS_HOST_SS                   0x0C04
#define VMCS_HOST_DS                   0x0C06
#define VMCS_HOST_FS                   0x0C08
#define VMCS_HOST_GS                   0x0C0A
#define VMCS_HOST_TR                   0x0C0C

/* 64-bit control */
#define VMCS_IO_BITMAP_A               0x2000
#define VMCS_IO_BITMAP_B               0x2002
#define VMCS_MSR_BITMAP                0x2004
#define VMCS_VM_EXIT_MSR_STORE_ADDR    0x2006
#define VMCS_VM_EXIT_MSR_LOAD_ADDR     0x2008
#define VMCS_VM_ENTRY_MSR_LOAD_ADDR    0x200A
#define VMCS_TSC_OFFSET                0x2010
#define VMCS_VIRTUAL_APIC_ADDR         0x2012
#define VMCS_APIC_ACCESS_ADDR          0x2014
#define VMCS_EPT_POINTER               0x201A

/* 64-bit VM-exit info */
#define VMCS_GUEST_PHYS_ADDR           0x2400

/* 64-bit guest state */
#define VMCS_VMCS_LINK_PTR             0x2800
#define VMCS_GUEST_IA32_DEBUGCTL       0x2802
#define VMCS_GUEST_IA32_PAT            0x2804
#define VMCS_GUEST_IA32_EFER           0x2806
#define VMCS_GUEST_PDPTE0              0x280A
#define VMCS_GUEST_PDPTE1              0x280C
#define VMCS_GUEST_PDPTE2              0x280E
#define VMCS_GUEST_PDPTE3              0x2810

/* 64-bit host state */
#define VMCS_HOST_IA32_PAT             0x2C00
#define VMCS_HOST_IA32_EFER            0x2C02

/* 32-bit control */
#define VMCS_PIN_BASED_CTLS            0x4000
#define VMCS_CPU_BASED_CTLS            0x4002
#define VMCS_EXCEPTION_BITMAP          0x4004
#define VMCS_PF_ERROR_CODE_MASK        0x4006
#define VMCS_PF_ERROR_CODE_MATCH       0x4008
#define VMCS_CR3_TARGET_COUNT          0x400A
#define VMCS_VM_EXIT_CTLS              0x400C
#define VMCS_VM_EXIT_MSR_STORE_COUNT   0x400E
#define VMCS_VM_EXIT_MSR_LOAD_COUNT    0x4010
#define VMCS_VM_ENTRY_CTLS             0x4012
#define VMCS_VM_ENTRY_MSR_LOAD_COUNT   0x4014
#define VMCS_VM_ENTRY_INTR_INFO        0x4016
#define VMCS_VM_ENTRY_EXCEPTION_ERR    0x4018
#define VMCS_VM_ENTRY_INSTR_LEN        0x401A
#define VMCS_CPU_BASED_CTLS2           0x401E

/* 32-bit VM-exit info */
#define VMCS_VM_INSTR_ERROR            0x4400
#define VMCS_VM_EXIT_REASON            0x4402
#define VMCS_VM_EXIT_INTR_INFO         0x4404
#define VMCS_VM_EXIT_INTR_ERROR        0x4406
#define VMCS_IDT_VECTORING_INFO        0x4408
#define VMCS_IDT_VECTORING_ERROR       0x440A
#define VMCS_VM_EXIT_INSTR_LEN         0x440C
#define VMCS_VMX_INSTR_INFO            0x440E

/* 32-bit guest state */
#define VMCS_GUEST_ES_LIMIT            0x4800
#define VMCS_GUEST_CS_LIMIT            0x4802
#define VMCS_GUEST_SS_LIMIT            0x4804
#define VMCS_GUEST_DS_LIMIT            0x4806
#define VMCS_GUEST_FS_LIMIT            0x4808
#define VMCS_GUEST_GS_LIMIT            0x480A
#define VMCS_GUEST_LDTR_LIMIT          0x480C
#define VMCS_GUEST_TR_LIMIT            0x480E
#define VMCS_GUEST_GDTR_LIMIT          0x4810
#define VMCS_GUEST_IDTR_LIMIT          0x4812
#define VMCS_GUEST_ES_AR               0x4814
#define VMCS_GUEST_CS_AR               0x4816
#define VMCS_GUEST_SS_AR               0x4818
#define VMCS_GUEST_DS_AR               0x481A
#define VMCS_GUEST_FS_AR               0x481C
#define VMCS_GUEST_GS_AR               0x481E
#define VMCS_GUEST_LDTR_AR             0x4820
#define VMCS_GUEST_TR_AR               0x4822
#define VMCS_GUEST_INTERRUPTIBILITY    0x4824
#define VMCS_GUEST_ACTIVITY_STATE      0x4826
#define VMCS_GUEST_SMBASE              0x4828
#define VMCS_GUEST_IA32_SYSENTER_CS    0x482A
#define VMCS_VMX_PREEMPT_TIMER_VAL     0x482E

/* 32-bit host state */
#define VMCS_HOST_IA32_SYSENTER_CS     0x4C00

/* Natural-width control */
#define VMCS_CR0_GUEST_HOST_MASK       0x6000
#define VMCS_CR4_GUEST_HOST_MASK       0x6002
#define VMCS_CR0_READ_SHADOW           0x6004
#define VMCS_CR4_READ_SHADOW           0x6006
#define VMCS_CR3_TARGET0               0x6008
#define VMCS_CR3_TARGET1               0x600A
#define VMCS_CR3_TARGET2               0x600C
#define VMCS_CR3_TARGET3               0x600E

/* Natural-width VM-exit info */
#define VMCS_EXIT_QUALIFICATION        0x6400
#define VMCS_GUEST_LINEAR_ADDR         0x640A

/* Natural-width guest state */
#define VMCS_GUEST_CR0                 0x6800
#define VMCS_GUEST_CR3                 0x6802
#define VMCS_GUEST_CR4                 0x6804
#define VMCS_GUEST_ES_BASE             0x6806
#define VMCS_GUEST_CS_BASE             0x6808
#define VMCS_GUEST_SS_BASE             0x680A
#define VMCS_GUEST_DS_BASE             0x680C
#define VMCS_GUEST_FS_BASE             0x680E
#define VMCS_GUEST_GS_BASE             0x6810
#define VMCS_GUEST_LDTR_BASE           0x6812
#define VMCS_GUEST_TR_BASE             0x6814
#define VMCS_GUEST_GDTR_BASE           0x6816
#define VMCS_GUEST_IDTR_BASE           0x6818
#define VMCS_GUEST_DR7                 0x681A
#define VMCS_GUEST_RSP                 0x681C
#define VMCS_GUEST_RIP                 0x681E
#define VMCS_GUEST_RFLAGS              0x6820
#define VMCS_GUEST_PENDING_DBG_EXCEPT  0x6822
#define VMCS_GUEST_IA32_SYSENTER_ESP   0x6824
#define VMCS_GUEST_IA32_SYSENTER_EIP   0x6826

/* Natural-width host state */
#define VMCS_HOST_CR0                  0x6C00
#define VMCS_HOST_CR3                  0x6C02
#define VMCS_HOST_CR4                  0x6C04
#define VMCS_HOST_FS_BASE              0x6C06
#define VMCS_HOST_GS_BASE              0x6C08
#define VMCS_HOST_TR_BASE              0x6C0A
#define VMCS_HOST_GDTR_BASE            0x6C0C
#define VMCS_HOST_IDTR_BASE            0x6C0E
#define VMCS_HOST_IA32_SYSENTER_ESP    0x6C10
#define VMCS_HOST_IA32_SYSENTER_EIP    0x6C12
#define VMCS_HOST_RSP                  0x6C14
#define VMCS_HOST_RIP                  0x6C16

/* -----------------------------------------------------------------------
 * Guest segment access rights helpers
 * ----------------------------------------------------------------------- */
#define VMCS_AR_UNUSABLE               (1U << 16)
#define VMCS_AR_S_BIT                  (1U << 4)    /* 1 = code/data descriptor */
#define VMCS_AR_PRESENT                (1U << 7)
#define VMCS_AR_L_BIT                  (1U << 13)   /* 64-bit code segment */
#define VMCS_AR_DB_BIT                 (1U << 14)   /* 32-bit default op size */
#define VMCS_AR_G_BIT                  (1U << 15)   /* 4K granularity */

/* Access rights for a real-mode data segment (type=3: expand-up r/w, accessed) */
#define VMCS_AR_REALMODE_DATA          (VMCS_AR_S_BIT | VMCS_AR_PRESENT | 0x3)
/* Access rights for a real-mode code segment (type=11: execute/read, accessed) */
#define VMCS_AR_REALMODE_CODE          (VMCS_AR_S_BIT | VMCS_AR_PRESENT | 0xB)
/* Access rights for a 64-bit LDT (type=2: LDT descriptor) */
#define VMCS_AR_LDTR_64                (VMCS_AR_PRESENT | 0x2)
/* Access rights for a 64-bit TSS active (type=11: 64-bit TSS available) */
#define VMCS_AR_TSS_64                 (VMCS_AR_PRESENT | 0xB)

/* -----------------------------------------------------------------------
 * EPT (Extended Page Tables)
 * ----------------------------------------------------------------------- */
#define EPTP_MEMTYPE_WB                (6ULL)
#define EPTP_PAGE_WALK_4               (3ULL << 3)   /* 4-level walk */
#define EPTP_ENABLE_AD                 (1ULL << 6)   /* accessed/dirty bits */

#define EPT_READ                       (1ULL << 0)
#define EPT_WRITE                      (1ULL << 1)
#define EPT_EXEC                       (1ULL << 2)
#define EPT_RWX                        (EPT_READ | EPT_WRITE | EPT_EXEC)
#define EPT_MEMTYPE_WB                 (6ULL << 3)
#define EPT_LARGE_PAGE                 (1ULL << 7)   /* 2MB page in EPD */
#define EPT_PHYS_MASK                  0x000FFFFFFFFFF000ULL

/* -----------------------------------------------------------------------
 * VMXON / VMCS regions
 * ----------------------------------------------------------------------- */
#define VMCS_LINK_PTR_INVALID          0xFFFFFFFFFFFFFFFFULL

/*
 * ----------------------------------------------------------------------- */
#define VMX_REGION_SIZE                PAGE_SIZE_4K
#define __aligned_4k                   __attribute__((aligned(PAGE_SIZE_4K)))

/* VMXON region: first 4 bytes must hold the VMX revision ID */
struct __aligned_4k VmxonRegion {
    uint32_t vmxon_revision_id;
    uint8_t  vmxon_data[VMX_REGION_SIZE - sizeof(uint32_t)];
};

/* -----------------------------------------------------------------------
 * Hypercall interface
 * ----------------------------------------------------------------------- */
#define HV_HYPERCALL_SIGNATURE         0x48594F56ULL  /* "HYOV" in RAX */
#define HV_HYPERCALL_MAGIC             0xDEADBEEFULL  /* response in RBX */

/* -----------------------------------------------------------------------
 * Saved guest general-purpose registers
 * Layout must match the push order in vmx_exit_trampoline.
 * ----------------------------------------------------------------------- */
struct __packed GuestRegs {
    uint64_t rax, rbx, rcx, rdx, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
};

/* -----------------------------------------------------------------------
 * Parsed VMX capability (filled by vmx_read_capabilities)
 * ----------------------------------------------------------------------- */
struct VmxCapabilities {
    uint32_t vc_revision_id;
    uint16_t vc_vmcs_size;
    bool     vc_true_ctls;          /* TRUE MSRs available */
    bool     vc_ept;                /* EPT supported */
    bool     vc_vpid;               /* VPID supported */
    bool     vc_unrestricted_guest; /* Unrestricted guest (real mode) */
    uint32_t vc_pin_ctls;           /* Adjusted pin-based controls */
    uint32_t vc_proc_ctls;          /* Adjusted primary proc-based controls */
    uint32_t vc_proc_ctls2;         /* Adjusted secondary proc-based controls */
    uint32_t vc_exit_ctls;          /* Adjusted VM-exit controls */
    uint32_t vc_entry_ctls;         /* Adjusted VM-entry controls */
};

/* -----------------------------------------------------------------------
 * Per-vCPU VMX state
 * ----------------------------------------------------------------------- */
struct VmxState {
    struct VmxonRegion    *vs_vmxon;       /* VMXON region (VA == PA, identity map) */
    void                  *vs_vmcs;        /* VMCS region (VA == PA, identity map) */
    struct VmxCapabilities vs_caps;
    uint64_t               vs_exit_count;  /* total VM exits handled */
};

/* -----------------------------------------------------------------------
 * 64-bit CR read helpers (system.h only has 32-bit versions)
 * ----------------------------------------------------------------------- */
static inline uint64_t cr0_read64(void)
{
    uint64_t v;
    __asm__ __volatile__("movq %%cr0, %0" : "=r"(v));
    return v;
}

static inline uint64_t cr3_read64(void)
{
    uint64_t v;
    __asm__ __volatile__("movq %%cr3, %0" : "=r"(v));
    return v;
}

static inline uint64_t cr4_read64(void)
{
    uint64_t v;
    __asm__ __volatile__("movq %%cr4, %0" : "=r"(v));
    return v;
}

static inline void cr4_write64(uint64_t v)
{
    __asm__ __volatile__("movq %0, %%cr4" : : "r"(v));
}

static inline void cr0_write64(uint64_t v)
{
    __asm__ __volatile__("movq %0, %%cr0" : : "r"(v));
}

/* -----------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------- */

/* vmx_ll.asm — raw VMX instruction wrappers */
int      vmx_on(uint64_t *vmxon_pa);
void     vmx_off(void);
int      vmx_clear(uint64_t *vmcs_pa);
int      vmx_ptr_load(uint64_t *vmcs_pa);
uint64_t vmx_read(uint32_t field);
void     vmx_write(uint32_t field, uint64_t value);
int      vmx_launch(void);
int      vmx_resume(void);
void     vmx_exit_trampoline(void);

/* vmx.c */
int  vmx_check_support(void);
int  vmx_read_capabilities(struct VmxCapabilities *caps);
int  vmx_enable(struct VmxState *state);
void vmx_print_info(struct VmxCapabilities *caps);

/* vmcs.c */
int vmcs_init(struct VmxState *state);

/* ept.c */
uint64_t ept_build(void);

/* vmx_exit.c */
void vmx_exit_handler(struct GuestRegs *regs);

#endif /* VMX_H */
