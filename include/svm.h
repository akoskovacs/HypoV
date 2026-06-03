/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | AMD SVM (AMD-V) definitions and VMCB layout                |
 * +------------------------------------------------------------+
*/
#ifndef SVM_H
#define SVM_H

#include <basic.h>
#include <types.h>
#include <memory.h>
#include <vmx.h>   /* reuse GuestRegisters, HV_HYPERCALL_* */

/* -----------------------------------------------------------------------
 * SVM MSRs
 * ----------------------------------------------------------------------- */
#define MSR_VM_CR                   0xC0010114
#define MSR_VM_HSAVE_PA             0xC0010117
#define VM_CR_SVMDIS                (1ULL << 4)  /* SVM disabled in BIOS */
#define EFER_SVME                   (1ULL << 12) /* SVM enable in EFER */

/* -----------------------------------------------------------------------
 * SVM exit codes (VMCB control.exit_code)
 * ----------------------------------------------------------------------- */
#define SVM_EXIT_RDTSC              0x6E
#define SVM_EXIT_CPUID              0x72
#define SVM_EXIT_HLT                0x78
#define SVM_EXIT_IOIO               0x7B
#define SVM_EXIT_VMMCALL            0x81
#define SVM_EXIT_RDTSCP             0x87
#define SVM_EXIT_SHUTDOWN           0x7F
#define SVM_EXIT_NPF                0x400  /* Nested Page Fault */
#define SVM_EXIT_INVALID            0xFFFFFFFFFFFFFFFFULL

/* -----------------------------------------------------------------------
 * VMCB control area intercept bits
 * ----------------------------------------------------------------------- */
/* intercept_misc1 */
#define SVM_ICEPT_RDTSC             (1U << 14)
#define SVM_ICEPT_CPUID             (1U << 18)
#define SVM_ICEPT_HLT               (1U << 24)
#define SVM_ICEPT_IOIO              (1U << 27)
#define SVM_ICEPT_SHUTDOWN          (1U << 31)

/* intercept_misc2 */
#define SVM_ICEPT_VMRUN             (1U << 0)  /* always intercept VMRUN */
#define SVM_ICEPT_VMMCALL           (1U << 1)
#define SVM_ICEPT_RDTSCP            (1U << 7)

/* -----------------------------------------------------------------------
 * IOIO exit qualification (exit_info1) bits
 * ----------------------------------------------------------------------- */
#define SVM_IOIO_TYPE_IN            (1U << 0)  /* 1=IN, 0=OUT */
#define SVM_IOIO_SZ8                (1U << 4)
#define SVM_IOIO_SZ16               (1U << 5)
#define SVM_IOIO_SZ32               (1U << 6)
#define SVM_IOIO_PORT_SHIFT         16

/* -----------------------------------------------------------------------
 * VMCB segment descriptor (16 bytes, offset from start of state save area)
 * ----------------------------------------------------------------------- */
struct __packed SvmSegment {
    uint16_t sel;
    uint16_t attrib;   /* descriptor bytes 5 and 6, limit field removed */
    uint32_t limit;
    uint64_t base;
};

/* Real-mode attrib values (P=1, S=1/0, DPL=0, type) */
#define SVM_ATTRIB_REALMODE_CODE    0x009B  /* P, S, type=0xB (exec/read/acc) */
#define SVM_ATTRIB_REALMODE_DATA    0x0093  /* P, S, type=0x3 (data r/w acc)  */
#define SVM_ATTRIB_TR               0x0083  /* P, S=0, type=3 (16-bit TSS)    */
#define SVM_ATTRIB_LDTR             0x0082  /* P, S=0, type=2 (LDT)           */
#define SVM_ATTRIB_UNUSABLE         0x0000  /* P=0 → unusable                 */

/* -----------------------------------------------------------------------
 * VMCB control area (0x000 – 0x3FF)
 * ----------------------------------------------------------------------- */
struct __packed SvmControl {
    uint32_t icept_cr;          /* 0x000  CR read[15:0] / write[31:16] */
    uint32_t icept_dr;          /* 0x004 */
    uint32_t icept_exceptions;  /* 0x008 */
    uint32_t icept_misc1;       /* 0x00C  CPUID, HLT, IO, etc. */
    uint32_t icept_misc2;       /* 0x010  VMMCALL, VMRUN, etc. */
    uint8_t  _pad0[0x28];       /* 0x014 – 0x03B */
    uint16_t pause_filter_thr;  /* 0x03C */
    uint16_t pause_filter_cnt;  /* 0x03E */
    uint64_t iopm_base_pa;      /* 0x040 */
    uint64_t msrpm_base_pa;     /* 0x048 */
    uint64_t tsc_offset;        /* 0x050 */
    uint32_t guest_asid;        /* 0x058  must be != 0 */
    uint32_t tlb_control;       /* 0x05C */
    uint64_t vintr;             /* 0x060 */
    uint64_t interrupt_shadow;  /* 0x068 */
    uint64_t exit_code;         /* 0x070 */
    uint64_t exit_info1;        /* 0x078 */
    uint64_t exit_info2;        /* 0x080 */
    uint64_t exit_int_info;     /* 0x088 */
    uint64_t np_enable;         /* 0x090  bit 0 = enable NPT */
    uint8_t  _pad1[0x10];       /* 0x098 – 0x0A7 */
    uint64_t event_inject;      /* 0x0A8 */
    uint64_t n_cr3;             /* 0x0B0  nested page table root PA */
    uint8_t  _pad2[0x2E8];      /* 0x0B8 – 0x3FF */
};                              /* total: 0x400 bytes */

/* -----------------------------------------------------------------------
 * VMCB state save area (0x400 – 0xFFF)
 * ----------------------------------------------------------------------- */
struct __packed SvmStateSave {
    struct SvmSegment es;       /* 0x400 */
    struct SvmSegment cs;       /* 0x410 */
    struct SvmSegment ss;       /* 0x420 */
    struct SvmSegment ds;       /* 0x430 */
    struct SvmSegment fs;       /* 0x440 */
    struct SvmSegment gs;       /* 0x450 */
    struct SvmSegment gdtr;     /* 0x460 */
    struct SvmSegment ldtr;     /* 0x470 */
    struct SvmSegment idtr;     /* 0x480 */
    struct SvmSegment tr;       /* 0x490 */
    uint8_t  _pad0[43];         /* 0x4A0 – 0x4CA */
    uint8_t  cpl;               /* 0x4CB */
    uint8_t  _pad1[4];          /* 0x4CC – 0x4CF */
    uint64_t efer;              /* 0x4D0 */
    uint8_t  _pad2[112];        /* 0x4D8 – 0x547 */
    uint64_t cr4;               /* 0x548 */
    uint64_t cr3;               /* 0x550 */
    uint64_t cr0;               /* 0x558 */
    uint64_t dr7;               /* 0x560 */
    uint64_t dr6;               /* 0x568 */
    uint64_t rflags;            /* 0x570 */
    uint64_t rip;               /* 0x578 */
    uint8_t  _pad3[88];         /* 0x580 – 0x5D7 */
    uint64_t rsp;               /* 0x5D8 */
    uint8_t  _pad4[24];         /* 0x5E0 – 0x5F7 */
    uint64_t rax;               /* 0x5F8 */
    uint64_t star;              /* 0x600 */
    uint64_t lstar;             /* 0x608 */
    uint64_t cstar;             /* 0x610 */
    uint64_t sfmask;            /* 0x618 */
    uint64_t kernel_gs_base;    /* 0x620 */
    uint64_t sysenter_cs;       /* 0x628 */
    uint64_t sysenter_esp;      /* 0x630 */
    uint64_t sysenter_eip;      /* 0x638 */
    uint64_t cr2;               /* 0x640 */
    uint8_t  _pad5[32];         /* 0x648 – 0x667 */
    uint64_t g_pat;             /* 0x668 */
    uint64_t dbgctl;            /* 0x670 */
    uint64_t br_from;           /* 0x678 */
    uint64_t br_to;             /* 0x680 */
    uint64_t last_excp_from;    /* 0x688 */
    uint64_t last_excp_to;      /* 0x690 */
    uint8_t  _pad6[0x370];      /* pad to 4KB total */
};

/* -----------------------------------------------------------------------
 * Full VMCB (4KB)
 * ----------------------------------------------------------------------- */
struct __aligned_4k Vmcb {
    struct SvmControl   control;
    struct SvmStateSave state;
};

/* -----------------------------------------------------------------------
 * Per-vCPU SVM state
 * ----------------------------------------------------------------------- */
struct SvmState {
    struct Vmcb          *ss_vmcb;
    uint64_t              ss_exit_count;
    struct GuestRegisters ss_guest_regs;
};

/* -----------------------------------------------------------------------
 * svm_ll.asm — raw SVM instruction wrappers
 * ----------------------------------------------------------------------- */
/* Execute VMRUN with VMCB at vmcb_pa; save/restore guest GPRs via *regs */
void svm_vmrun(uint64_t vmcb_pa, struct GuestRegisters *regs);

/* -----------------------------------------------------------------------
 * svm.c
 * ----------------------------------------------------------------------- */
int  svm_check_support(void);
int  svm_enable(struct SvmState *state);
void svm_run_guest(struct SvmState *state);
void svm_print_info(void);

/* -----------------------------------------------------------------------
 * npt.c
 * ----------------------------------------------------------------------- */
uint64_t npt_build(void);

#endif /* SVM_H */
