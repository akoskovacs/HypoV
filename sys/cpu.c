/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | CPU set up, feature detection and mode setting             |
 * +------------------------------------------------------------+
*/
#include <cpu.h>
#include <error.h>
#include <basic.h>
#include <system.h>
#include <string.h>
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

extern void __cpu_long_mode_enter(void);

/*
 * Going 64bit, baby...
*/
int cpu_init_long_mode(struct SystemInfo *info)
{
    int error = 0;
    /* Enable long-mode. 64bit will not be active, until paging is not set up. */
    uint64_t efer = msr_read(MSR_IA32_EFER) | EFER_LME;
    msr_write(MSR_IA32_EFER, efer);
    
    /* Don't do anything if we are already in 64bit mode */
    if (efer & EFER_LMA) {
        return -HV_EWONTDO;
    }

    /* Setup all the paging levels needed for now */
    error = mm_init_paging(info);
    if (error != 0) {
        return error;
    }

    __cpu_long_mode_enter();

    /* Check if 64bit mode is activated */
    if (!(msr_read(MSR_IA32_EFER) & EFER_LMA)) {
        return -HV_GENERIC;
    }

    /* Success, everything is much better in 64bit. :)
       Unfortunately, it's still the compatibility submode. :( */
    return 0;
}

int gdt_make_entry(struct GDTEntry *ent, uint32_t base, uint32_t limit, uint32_t flags)
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

extern void __gdt_setup_32(unsigned long limit, unsigned long base);
extern void __tss_setup_32(uint16_t tss_selector);

#define GDT_CODE16_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ)
#define GDT_DATA16_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE)

#define GDT_CODE32_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_32BIT | DESC_GRANULAR)
#define GDT_DATA32_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE | DESC_32BIT | DESC_GRANULAR)

#define GDT_CODE64_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_64BIT)
#define GDT_DATA64_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE | DESC_64BIT)

static struct GDTEntry  gdt_table[GDT_NR_ENTRIES] __aligned_16;
static struct TSS32     tss_sys_32 __aligned_16;
static struct TSS64     tss_sys_64 __aligned_16;

#define TSS_BUSY        0xB00
#define TSS_AVAILABLE   0x900

int cpu_tables_init(void)
{
    bzero(&tss_sys_32, sizeof(tss_sys_32));
    bzero(&tss_sys_64, sizeof(tss_sys_64));
    gdt_make_entry(gdt_table + GDT_SYS_NULL, 0x0, 0x0, 0x0);

    gdt_make_entry(gdt_table + GDT_SYS_CODE16, 0x0, GDT_LIMIT_MAX16, GDT_CODE16_FLAGS);
    gdt_make_entry(gdt_table + GDT_SYS_DATA16, 0x0, GDT_LIMIT_MAX16, GDT_DATA16_FLAGS);
    bochs_breakpoint();
    gdt_make_entry(gdt_table + GDT_SYS_CODE32, 0x0, GDT_LIMIT_MAX32, GDT_CODE32_FLAGS);
    gdt_make_entry(gdt_table + GDT_SYS_DATA32, 0x0, GDT_LIMIT_MAX32, GDT_DATA32_FLAGS);

    gdt_make_entry(gdt_table + GDT_SYS_CODE64, 0x0, GDT_LIMIT_MAX32, GDT_CODE64_FLAGS);
    gdt_make_entry(gdt_table + GDT_SYS_DATA64, 0x0, GDT_LIMIT_MAX32, GDT_DATA64_FLAGS);

    gdt_make_entry(gdt_table + GDT_SYS_TSS32, (uint32_t)&tss_sys_32, sizeof(tss_sys_32), DESC_PRESENT | TSS_AVAILABLE);
    /* The size of the 64bit GDT entry is twice of the legacy ones (needs two entries),
       the second part could be entirely zero, because the TSS address will be under 4GB */
    /* XXX: Must be the last entry */
    gdt_make_entry(gdt_table + GDT_SYS_TSS64, (uint32_t)&tss_sys_64, sizeof(tss_sys_64), DESC_PRESENT | TSS_AVAILABLE);
    gdt_make_entry(gdt_table + GDT_SYS_TSS64 + 1, 0x0, 0x0, 0x0);

    __gdt_setup_32(GDT_NR_ENTRIES * sizeof(gdt_table[0]), (unsigned long)&gdt_table);
    __tss_setup_32(GDT_SEL(GDT_SYS_TSS32));
    return 0; 
}