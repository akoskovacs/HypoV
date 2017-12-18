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
#define CPU_FEATURE_FPU         (1L << 0) /* Onboard x87 FPU */
#define CPU_FEATURE_VME         (1L << 1) /* Virtual 8086 mode */
#define CPU_FEATURE_DE          (1L << 2) /* Debugging extensions */
#define CPU_FEATURE_PSE         (1L << 3) 
#define CPU_FEATURE_TSC         (1L << 4) 
#define CPU_FEATURE_MSR         (1L << 5) 
#define CPU_FEATURE_PAE         (1L << 6) 
#define CPU_FEATURE_MCE         (1L << 7) 
#define CPU_FEATURE_CX8         (1L << 8) 
#define CPU_FEATURE_APIC        (1L << 9) 
// Reserved bit
#define CPU_FEATURE_SEP         (1L << 11)
#define CPU_FEATURE_MTRR        (1L << 12)
#define CPU_FEATURE_PGE         (1L << 13)
#define CPU_FEATURE_MCA         (1L << 14)
#define CPU_FEATURE_CMOV        (1L << 15)
#define CPU_FEATURE_PAT         (1L << 16)
#define CPU_FEATURE_PSE36       (1L << 17)
#define CPU_FEATURE_PSN         (1L << 18)
#define CPU_FEATURE_CLFSH       (1L << 19)
// Reserved bit
#define CPU_FEATURE_DS          (1L << 21)
#define CPU_FEATURE_ACPI        (1L << 22)
#define CPU_FEATURE_MMX         (1L << 23)
#define CPU_FEATURE_FXSR        (1L << 24)
#define CPU_FEATURE_SSE         (1L << 25)
#define CPU_FEATURE_SSE2        (1L << 26)
#define CPU_FEATURE_SS          (1L << 27)
#define CPU_FEATURE_HT          (1L << 28)
#define CPU_FEATURE_TM          (1L << 29)
#define CPU_FEATURE_IA64        (1L << 30)
#define CPU_FEATURE_PBE         (1L << 31)

/* EAX = 1, ECX = ... */
#define CPU_FEATURE_SSE3        (1L << 32)
#define CPU_FEATURE_PCMULDQDQ   (1L << 33)
#define CPU_FEATURE_DTES64      (1L << 34)
#define CPU_FEATURE_MONITOR     (1L << 35)
#define CPU_FEATURE_DS_CPL      (1L << 36)
#define CPU_FEATURE_VMX         (1L << 37) /* Virtual mode extensions */
#define CPU_FEATURE_SMX         (1L << 38)
#define CPU_FEATURE_EST         (1L << 39)
#define CPU_FEATURE_TM2         (1L << 40)
#define CPU_FEATURE_SSSE3       (1L << 41)
#define CPU_FEATURE_CNXT_ID     (1L << 42)
#define CPU_FEATURE_SDBG        (1L << 43)
#define CPU_FEATURE_FMA         (1L << 44)
#define CPU_FEATURE_CX16        (1L << 45)
#define CPU_FEATURE_XTPR        (1L << 46)
#define CPU_FEATURE_PDCM        (1L << 47)
// Reserved bit
#define CPU_FEATURE_PCID        (1L << 49)
#define CPU_FEATURE_DCA         (1L << 50)
#define CPU_FEATURE_SSE4_1      (1L << 51)
#define CPU_FEATURE_SSE4_2      (1L << 52)
#define CPU_FEATURE_X2APIC      (1L << 53)
#define CPU_FEATURE_MOVBE       (1L << 54)
#define CPU_FEATURE_POPCNT      (1L << 55)
#define CPU_FEATURE_TSC_DLINE   (1L << 56)
#define CPU_FEATURE_AES         (1L << 57)
#define CPU_FEATURE_XSAVE       (1L << 58)
#define CPU_FEATURE_OSXSAVE     (1L << 59)
#define CPU_FEATURE_AVX         (1L << 60)
#define CPU_FEATURE_F16C        (1L << 61)
#define CPU_FEATURE_RDRND       (1L << 62)
#define CPU_FEATURE_HVISOR      (1L << 63)

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