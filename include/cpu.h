/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | CPU type and feature detection                             |
 * +------------------------------------------------------------+
*/
#ifndef CPU_H
#define CPU_H

#include <basic.h>
#include <types.h>
#include <gdt.h>

#define CPU_VENDOR_SIZE         12
#define CPU_BRANDING_SIZE       48

/* Feature flags got from the CPUID instruction */

/* EAX = 1, EDX = ... */
enum CPU_FEATURE_FLAGS {
    CPU_FEATURE_FPU        = (1ULL << 0), /* Onboard x87 FPU */
    CPU_FEATURE_VME        = (1ULL << 1), /* Virtual 8086 mode */
    CPU_FEATURE_DE         = (1ULL << 2), /* Debugging extensions */
    CPU_FEATURE_PSE        = (1ULL << 3), 
    CPU_FEATURE_TSC        = (1ULL << 4), 
    CPU_FEATURE_MSR        = (1ULL << 5), 
    CPU_FEATURE_PAE        = (1ULL << 6), 
    CPU_FEATURE_MCE        = (1ULL << 7), 
    CPU_FEATURE_CX8        = (1ULL << 8), 
    CPU_FEATURE_APIC       = (1ULL << 9), 
    // Reserved bit
    CPU_FEATURE_SEP        = (1ULL << 11),
    CPU_FEATURE_MTRR       = (1ULL << 12),
    CPU_FEATURE_PGE        = (1ULL << 13),
    CPU_FEATURE_MCA        = (1ULL << 14),
    CPU_FEATURE_CMOV       = (1ULL << 15),
    CPU_FEATURE_PAT        = (1ULL << 16),
    CPU_FEATURE_PSE36      = (1ULL << 17),
    CPU_FEATURE_PSN        = (1ULL << 18),
    CPU_FEATURE_CLFSH      = (1ULL << 19),
    // Reserved bit
    CPU_FEATURE_DS         = (1ULL << 21),
    CPU_FEATURE_ACPI       = (1ULL << 22),
    CPU_FEATURE_MMX        = (1ULL << 23),
    CPU_FEATURE_FXSR       = (1ULL << 24),
    CPU_FEATURE_SSE        = (1ULL << 25),
    CPU_FEATURE_SSE2       = (1ULL << 26),
    CPU_FEATURE_SS         = (1ULL << 27),
    CPU_FEATURE_HT         = (1ULL << 28),
    CPU_FEATURE_TM         = (1ULL << 29),
    CPU_FEATURE_IA64       = (1ULL << 30),
    CPU_FEATURE_PBE        = (1ULL << 31),
    /* EAX = 1, ECX = ... */
    CPU_FEATURE_SSE3       = (1ULL << 32),
    CPU_FEATURE_PCMULDQDQ  = (1ULL << 33),
    CPU_FEATURE_DTES64     = (1ULL << 34),
    CPU_FEATURE_MONITOR    = (1ULL << 35),
    CPU_FEATURE_DS_CPL     = (1ULL << 36),
    CPU_FEATURE_VMX        = (1ULL << 37), /* Virtual mode extensions */
    CPU_FEATURE_SMX        = (1ULL << 38),
    CPU_FEATURE_EST        = (1ULL << 39),
    CPU_FEATURE_TM2        = (1ULL << 40),
    CPU_FEATURE_SSSE3      = (1ULL << 41),
    CPU_FEATURE_CNXT_ID    = (1ULL << 42),
    CPU_FEATURE_SDBG       = (1ULL << 43),
    CPU_FEATURE_FMA        = (1ULL << 44),
    CPU_FEATURE_CX16       = (1ULL << 45),
    CPU_FEATURE_XTPR       = (1ULL << 46),
    CPU_FEATURE_PDCM       = (1ULL << 47),
    // Reserved bit
    CPU_FEATURE_PCID       = (1ULL << 49),
    CPU_FEATURE_DCA        = (1ULL << 50),
    CPU_FEATURE_SSE4_1     = (1ULL << 51),
    CPU_FEATURE_SSE4_2     = (1ULL << 52),
    CPU_FEATURE_X2APIC     = (1ULL << 53),
    CPU_FEATURE_MOVBE      = (1ULL << 54),
    CPU_FEATURE_POPCNT     = (1ULL << 55),
    CPU_FEATURE_TSC_DLINE  = (1ULL << 56),
    CPU_FEATURE_AES        = (1ULL << 57),
    CPU_FEATURE_XSAVE      = (1ULL << 58),
    CPU_FEATURE_OSXSAVE    = (1ULL << 59),
    CPU_FEATURE_AVX        = (1ULL << 60),
    CPU_FEATURE_F16C       = (1ULL << 61),
    CPU_FEATURE_RDRND      = (1ULL << 62),
    CPU_FEATURE_HVISOR     = (1ULL << 63)
};
    
enum CR0_FLAGS {
    CR0_PE  = (1ULL << 0), /* Protected mode enable */
    CR0_MP  = (1ULL << 1), /* Monitor co-processor */
    CR0_EM  = (1ULL << 2), /* Emulation */
    CR0_TS  = (1ULL << 3), /* Task switched */
    CR0_ET  = (1ULL << 4), /* Extension type */
    CR0_NE  = (1ULL << 5), /* Numberic error */
    // Reserved (6..15)
    CR0_WP  = (1ULL << 16), /* Write protect */
    // Reserved
    CR0_AM  = (1ULL << 18), /* Alignment mask */
    // Reserved (19..28)
    CR0_NW  = (1ULL << 29), /* No-write through */
    CR0_CD  = (1ULL << 30), /* Cache disable */
    CR0_PG  = (1ULL << CR0_PG_BIT), /* Paging enable */
    // Reserved (32..63)
};

/* Structures and flags for the different control registers */

/* 
 * This structure holds the Page Directory Table base address,
 * when paging is enabled. We use 2Mbyte pages, therefore only
 * three page levels needed, even in 64bit mode.
*/
struct __packed Cr3
{
    union {
        /* Active if CR4_PCIDE = 0 */
        struct {
            uint8_t reserved0           : 2;
            uint8_t pg_wr_through       : 1;
            uint8_t pg_cache_disable    : 1;
            uint8_t reserved5_11        : 7;
        } pcid_off;
        /* Active when CR4_PCIDE = 1 */
        uint32_t   pcid                 : 11;
    } bits;
    uint64_t pml4_pa                    : 40; // (12..51) 4K aligned
    uint32_t reserved52_63              : 13; 
};

enum CR4_FLAGS {
    CR4_VME        = (1ULL << 0),  /* Virtual-8086 (not hypervisor stuff) */
    CR4_PVI        = (1ULL << 1),  /* Protected mode virtual interrupts */
    CR4_TSD        = (1ULL << 2),  /* Time-stamp only in ring0 */
    CR4_DE         = (1ULL << 3),  /* Debug extensions */
    CR4_PSE        = (1ULL << 4),  /* Page size extensions */
    CR4_PAE        = (1ULL << 5),  /* Physical address extensions */
    CR4_MCE        = (1ULL << 6),  /* Machine check exception */
    CR4_PGE        = (1ULL << 7),  /* Page global enable */
    CR4_PCE        = (1ULL << 8),  /* Performance monitoring enable */
    CR4_OSFXSR     = (1ULL << 9),  /* Support for FXSAVE, FXSTOR instructions */
    CR4_OSXMMEXCPT = (1ULL << 10), /* Support for unmasked floating point SIMD instructions */
    CR4_UMIP       = (1ULL << 11), /* Virtual machine hardware assist enable */
    // Reserved
    CR4_VMXE       = (1ULL << 13), /* Virtual machine hardware assist enable */
    CR4_SMXE       = (1ULL << 14), /* Safe mode extensions */
    // Reserved (15..17)
    CR4_PCIDE      = (1ULL << 17), /* PCID enable */
    CR4_OSXSAVE    = (1ULL << 18), /* XSAVE and processor extend state enable */
    // Reserved
    CR4_SMEP       = (1ULL << 20), /* Supervisor Mode Executions Procetion enable */
    CR4_SMAP       = (1ULL << 21), /* Supervisor Mode Access Protection enable */
    // Reserved (22..63)
};

/* Extended Feature Enable Register (MSR), only for Intel now  */
#define MSR_IA32_EFER 0xC0000080

enum MSR_IA32_EFER_FLAGS {
    EFER_SCE       = (1ULL << 0),  /* System call extensions enable */
    // Reserved (1..7)
    EFER_LME       = (1ULL << 8),  /* 64-bit mode enable (Long Mode enable) */
    // Reserved
    EFER_LMA       = (1ULL << 10), /* Long mode active */
    EFER_NXE       = (1ULL << 11)  /* No-execute bit enable */
    // Reserved (11.63) on Intel processors
};

struct CpuInfo
{
    char    *ci_vendor;
    char    *ci_branding;
    int      ci_family;
    int      ci_stepping;
    int      ci_model;
    int      ci_processor_type;
    int      ci_ext_model_id;
    int      ci_ext_family_id;
    int      ci_brand_index; // TODO
    uint64_t ci_features;
};

struct __packed GDTEntry 
{
    uint16_t limit;
    uint16_t base_addr;
    uint32_t flags;
};

struct __packed TSS16
{
    uint16_t tss_link;
    uint16_t sp0;
    uint16_t ss0;
    uint16_t sp1;
    uint16_t ss1;
    uint16_t sp2;
    uint16_t ss2;
    uint16_t ip;
    uint16_t flag;
    uint16_t ax;
    uint16_t cx;
    uint16_t dx;
    uint16_t bx;
    uint16_t sp;
    uint16_t bp;
    uint16_t si;
    uint16_t di;
    uint16_t es;
    uint16_t cs;
    uint16_t ss;
    uint16_t ds;
    uint16_t ldt_selector;
};

/* 
 * Task State Segment for the 32 bit code.
 * Reserved fields are ignored by the hardware and has
 * to be set to 0. Only one needed, since multitasking
 * will not be used in any 32 bit code here.
 */
struct __packed TSS32
{
    uint16_t tss_reserved_0;
    /* Link to the next TSS (unused here) */
    uint16_t tss_link;

    /* Stack pointers and segments for ring 0, 1, 2 */
    uint32_t tss_esp0;

    uint16_t tss_reserved_1;
    uint16_t tss_ss0;

    uint32_t tss_esp1;

    uint16_t tss_reserved_2; 
    uint16_t tss_ss1; 

    uint32_t tss_esp2;

    uint16_t tss_reserved_3; 
    uint16_t ss2;

    /* Page level 3 pointer */
    uint32_t tss_cr3;
    /* Normal registers */
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;

    /* Segment registers */
    uint16_t tss_reserved_4;
    uint16_t es;

    uint16_t tss_reserved_5;
    uint16_t cs;

    uint16_t tss_reserved_6;
    uint16_t ss;

    uint16_t tss_reserved_7;
    uint16_t ds;

    uint16_t tss_reserved_8;
    uint16_t fs;

    uint16_t tss_reserved_9;
    uint16_t gs;

    uint16_t tss_reserved_10;
    /* Local Descriptor Table selector */
    uint16_t ldt_selector;

    /* I/O mapping base address */
    uint16_t iomap_base;

#define TSS32_FLAG_TRAP 0x1
    uint16_t flags;
};

struct __packed TSS64
{
    uint32_t tss_reserved_0;
    /* RSP for ring 0, 1, 2 */
    uint32_t rsp_0_low;
    uint32_t rsp_0_high;

    uint32_t rsp_1_low;
    uint32_t rsp_1_high;

    uint32_t rsp_2_low;
    uint32_t rsp_2_high;

    uint64_t tss_reserved_1;

    /* Interrupt Stack Table pointers */
    uint64_t ist[7];

    uint64_t tss_reserved_2;
    uint16_t tss_reserved_3;

    /* I/O mapping base address */
    uint16_t iomap_base;
};

struct SystemInfo;

int             cpu_tables_init(void);
struct CpuInfo *cpu_get_info(void);
int             cpu_set_info(struct CpuInfo *info);
int             cpu_init_long_mode(struct SystemInfo *info);

static inline int 
gdt_make_entry(struct GDTEntry *ent, uint32_t base, uint32_t limit, uint32_t flags)
{
    /* The base and limit cannot fit into these members alone,
       flags also contain some bits of these */
    ent->base_addr = base  & 0xFFFFU;
    ent->limit     = limit & 0xFFFFU;
    /* The flags member also contains base[31:24] = flags[31:24], base[23:16] = flags[7:0]
       and limit[19:16] = flags[19:16] */
    ent->flags     = (uint32_t)(flags | (base & 0xFF000000U) | ((base & 0x00FF0000U) >> 16) | (limit & 0xF0000U));
    return 0;
}

#endif // CPU_H