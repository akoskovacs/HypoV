/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Host CPU management                                        |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <cpu.h>
#include <string.h>
#include <system.h>

/* Helper subroutines from host_cpu_ll.asm */
extern unsigned long __get_relocation_offset(void);
extern void __gdt_setup_64(uint16_t seglimit, unsigned long base);
extern void __tss_setup_64(uint16_t tss_selector);
extern void __idt_setup_64(uint16_t seglimit, unsigned long base);

/* The full interrupt vector with auto-generated assembly wrappers */
extern hv_int_handler_ft __int_vector[];

#define NR_HV_GDT_ELEMS 5

static struct GDTEntry   gdt_table[NR_HV_GDT_ELEMS] __aligned_16;
static struct IDT64Entry idt_table[NR_HV_INTERRUPTS] __aligned_16;
static struct TSS64      tss_sys_64 __aligned_16;

/* Create TSS entry inside the GDT. Note that it needs 2 normal GDT entries, since it's a 64 bit entry. */
void gdt_make_tss_entry(struct GDTEntry *ent, uint64_t base, uint32_t limit, uint32_t flags)
{
    struct TSSEntry *tsse = (struct TSSEntry *)ent;
    gdt_make_entry(&tsse->entry, (uint32_t)base, limit, flags);
    tsse->base_32_63 = (uint32_t)(base >> 32) & 0xFFFFFFFF;
    tsse->reserved0  = 0;
}

static void cpu_gdt_init(void)
{
    /* Everything in the TSS could be initialized to be zero (including RSP0), meaning
     * that the interrupt handlers use the normal stack. */
    bzero((void *)&tss_sys_64, sizeof(tss_sys_64));

    /* Set up the final Global Descriptor Table */
    gdt_make_entry(gdt_table + GDT_HV_NULL, 0x0, 0x0, 0x0);
    gdt_make_entry(gdt_table + GDT_HV_CODE64, 0x0, GDT_LIMIT_MAX32, GDT_CODE64_FLAGS);
    gdt_make_entry(gdt_table + GDT_HV_DATA64, 0x0, GDT_LIMIT_MAX32, GDT_DATA64_FLAGS);
    /* The size of the 64bit TSS entry is twice the size of the legacy ones (needs two entries) */
    gdt_make_tss_entry(gdt_table + GDT_HV_TSS64, (uint64_t)&tss_sys_64
        , sizeof(tss_sys_64), DESC_PRESENT | TSS_AVAILABLE);
    __gdt_setup_64(sizeof(gdt_table) - 1, (unsigned long)&gdt_table);
    __tss_setup_64(GDT_SEL(GDT_HV_TSS64));
}

/*
 * Setup the full interrupt descriptor table for all the final interrupt handlers.
 * The IDT pointer points to the relocated, automatically generated assembly wrappers.
 */
static void cpu_idt_init(void)
{
    /* The table from the generated assembly code only knows "absolute" addresses, the
       current relocation offset must be added to it! */
    unsigned long rel_off = __get_relocation_offset();
    for (int i = 0; i < NR_HV_INTERRUPTS; i++) {
        idt64_make_entry(idt_table + i, false, GDT_SEL(GDT_HV_CODE64), rel_off + __int_vector[i]);
    }

    __idt_setup_64(sizeof(idt_table) - 1, (unsigned long)&idt_table);
}

void cpu_init_tables(void)
{
    cpu_gdt_init();
    cpu_idt_init();

    int_enable();
}
