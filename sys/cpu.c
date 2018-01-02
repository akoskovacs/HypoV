/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | CPU type and feature detection                             |
 * +------------------------------------------------------------+
*/
#include <cpu.h>
#include <error.h>
#include <basic.h>
#include <system.h>
#include <memory.h>

#define STEPPING_ID_MASK    0x0F       // 00000000001111
#define MODEL_MASK          0xF0       // 00000011110000
#define FAMILY_MASK         0xF00      // 00111100000000
#define PROCESSOR_MASK      0x3000     // 11000000000000
#define EXT_MODEL_MASK      0xF0000    // ...
#define EXT_FAMILY_MASK     0xFF00000  // ...

#define GET_STEPPING(eax)   ((eax) & STEPPING_ID_MASK)
#define GET_MODEL(eax)      (((eax) & MODEL_MASK) >> 4)
#define GET_FAMILY(eax)     (((eax) & FAMILY_MASK) >> 8)
#define GET_PROCESSOR(eax)  (((eax) & PROCESSOR_MASK) >> 12)
#define GET_EXT_MODEL(eax)  (((eax) & EXT_MODEL_MASK) >> 16)
#define GET_EXT_FAMILY(eax) (((eax) & EXT_FAMILY_MASK) >> 20)

struct CpuInfo *cpu_get_info(void)
{
    struct CpuInfo *info = (struct CpuInfo *)mm_expand_heap(sizeof(*info));
    if (info) {
        cpu_set_info(info);
    }
    return info;
}

int cpu_set_info(struct CpuInfo *info)
{
    char *branding;
    int32_t regs[4];
    if (info == NULL) {
        return -HV_BADARG;
    }

    info->ci_vendor   = (char *)mm_expand_heap(CPU_VENDOR_SIZE + 1);
    branding          = (char *)mm_expand_heap(CPU_BRANDING_SIZE + 1);

    cpuid_get_branding(branding);
    cpuid_get_vendor(info->ci_vendor);
    cpuid(0x1, regs);

    while (*branding == ' ') {
        branding++;
    }

    info->ci_branding = branding;
    info->ci_stepping = GET_STEPPING(regs[CPUID_REG_EAX]);
    info->ci_family   = GET_FAMILY(regs[CPUID_REG_EAX]);
    info->ci_model    = GET_MODEL(regs[CPUID_REG_EAX]);
    info->ci_features = regs[CPUID_REG_EDX] | (((uint64_t)regs[CPUID_REG_ECX]) << 32);

    return 0;
}

void __cpu_enter_long_mode(void);

/*
 * Going 64bit, baby...
*/
int cpu_init_long_mode(struct SystemInfo *info)
{
    int error = 0;
    /* Enable long-mode. 64bit will not be active, until paging is not set up. */
    uint64_t efer = msr_read(MSR_IA32_EFER) | EFER_LME;
    msr_write(MSR_IA32_EFER, efer);

    /* Setup all the paging levels needed for now */
    error = mm_init_paging(info);
    if (error != 0) {
        return error;
    }

#if 0
    uint32_t cr0 = cr0_read() | CR0_PG;
    bochs_breakpoint();
    cr0_write(cr0);
#endif

    __cpu_enter_long_mode();

    /* Check if 64bit mode is activated */
    if (!(msr_read(MSR_IA32_EFER) & EFER_LMA)) {
        return -HV_GENERIC;
    }

    /* Success, everything is much better in 64bit */
    return 0;
}
