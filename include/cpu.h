/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | CPU type and feature detection                             |
 * +------------------------------------------------------------+
*/
#ifndef CPU_H
#define CPU_H

#include <types.h>

#define CPU_VENDOR_SIZE         12
#define CPU_BRANDING_SIZE       48

/* EAX = 1, EDX = ... */
#define CPU_FEATURE_FPU         (1ULL << 0) /* Onboard x87 FPU */
#define CPU_FEATURE_VME         (1ULL << 1) /* Virtual 8086 mode */
#define CPU_FEATURE_DE          (1ULL << 2) /* Debugging extensions */
#define CPU_FEATURE_PSE         (1ULL << 3) 
#define CPU_FEATURE_TSC         (1ULL << 4) 
#define CPU_FEATURE_MSR         (1ULL << 5) 
#define CPU_FEATURE_PAE         (1ULL << 6) 
#define CPU_FEATURE_MCE         (1ULL << 7) 
#define CPU_FEATURE_CX8         (1ULL << 8) 
#define CPU_FEATURE_APIC        (1ULL << 9) 
// Reserved bit
#define CPU_FEATURE_SEP         (1ULL << 11)
#define CPU_FEATURE_MTRR        (1ULL << 12)
#define CPU_FEATURE_PGE         (1ULL << 13)
#define CPU_FEATURE_MCA         (1ULL << 14)
#define CPU_FEATURE_CMOV        (1ULL << 15)
#define CPU_FEATURE_PAT         (1ULL << 16)
#define CPU_FEATURE_PSE36       (1ULL << 17)
#define CPU_FEATURE_PSN         (1ULL << 18)
#define CPU_FEATURE_CLFSH       (1ULL << 19)
// Reserved bit
#define CPU_FEATURE_DS          (1ULL << 21)
#define CPU_FEATURE_ACPI        (1ULL << 22)
#define CPU_FEATURE_MMX         (1ULL << 23)
#define CPU_FEATURE_FXSR        (1ULL << 24)
#define CPU_FEATURE_SSE         (1ULL << 25)
#define CPU_FEATURE_SSE2        (1ULL << 26)
#define CPU_FEATURE_SS          (1ULL << 27)
#define CPU_FEATURE_HT          (1ULL << 28)
#define CPU_FEATURE_TM          (1ULL << 29)
#define CPU_FEATURE_IA64        (1ULL << 30)
#define CPU_FEATURE_PBE         (1ULL << 31)

/* EAX = 1, ECX = ... */
#define CPU_FEATURE_SSE3        (1ULL << 32)
#define CPU_FEATURE_PCMULDQDQ   (1ULL << 33)
#define CPU_FEATURE_DTES64      (1ULL << 34)
#define CPU_FEATURE_MONITOR     (1ULL << 35)
#define CPU_FEATURE_DS_CPL      (1ULL << 36)
#define CPU_FEATURE_VMX         (1ULL << 37) /* Virtual mode extensions */
#define CPU_FEATURE_SMX         (1ULL << 38)
#define CPU_FEATURE_EST         (1ULL << 39)
#define CPU_FEATURE_TM2         (1ULL << 40)
#define CPU_FEATURE_SSSE3       (1ULL << 41)
#define CPU_FEATURE_CNXT_ID     (1ULL << 42)
#define CPU_FEATURE_SDBG        (1ULL << 43)
#define CPU_FEATURE_FMA         (1ULL << 44)
#define CPU_FEATURE_CX16        (1ULL << 45)
#define CPU_FEATURE_XTPR        (1ULL << 46)
#define CPU_FEATURE_PDCM        (1ULL << 47)
// Reserved bit
#define CPU_FEATURE_PCID        (1ULL << 49)
#define CPU_FEATURE_DCA         (1ULL << 50)
#define CPU_FEATURE_SSE4_1      (1ULL << 51)
#define CPU_FEATURE_SSE4_2      (1ULL << 52)
#define CPU_FEATURE_X2APIC      (1ULL << 53)
#define CPU_FEATURE_MOVBE       (1ULL << 54)
#define CPU_FEATURE_POPCNT      (1ULL << 55)
#define CPU_FEATURE_TSC_DLINE   (1ULL << 56)
#define CPU_FEATURE_AES         (1ULL << 57)
#define CPU_FEATURE_XSAVE       (1ULL << 58)
#define CPU_FEATURE_OSXSAVE     (1ULL << 59)
#define CPU_FEATURE_AVX         (1ULL << 60)
#define CPU_FEATURE_F16C        (1ULL << 61)
#define CPU_FEATURE_RDRND       (1ULL << 62)
#define CPU_FEATURE_HVISOR      (1ULL << 63)

struct CpuInfo
{
    char               *ci_vendor;
    char               *ci_branding;
    int                 ci_family;
    int                 ci_stepping;
    int                 ci_model;
    int                 ci_processor_type;
    int                 ci_ext_model_id;
    int                 ci_ext_family_id;
    int                 ci_brand_index; // TODO
    uint64_t            ci_features;
};

struct CpuInfo *cpu_get_info(void);
int cpu_set_info(struct CpuInfo *info);

#endif // CPU_H