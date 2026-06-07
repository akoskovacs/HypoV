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
#include <interrupt.h>
#include <memory.h>
#include <vmx.h>

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

uint64_t cpu_get_gdt_base(void) { return (uint64_t)gdt_table; }
uint64_t cpu_get_idt_base(void) { return (uint64_t)idt_table; }
uint64_t cpu_get_tss_base(void) { return (uint64_t)&tss_sys_64; }

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
    gdt_make_entry(gdt_table + GDT_HV_CODE64, 0x0, 0x0, GDT_CODE64_FLAGS);
    gdt_make_entry(gdt_table + GDT_HV_DATA64, 0x0, 0x0, GDT_DATA64_FLAGS);
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
        idt64_make_entry(idt_table + i, true, GDT_SEL(GDT_HV_CODE64), rel_off + __int_vector[i]);
    }

    __idt_setup_64(sizeof(idt_table) - 1, (unsigned long)&idt_table);
}

void cpu_init_tables(void)
{
    cpu_gdt_init();
    cpu_idt_init();
}

#define HOST_PT_NR_ENTRIES 512
#define HOST_PT_NR_PDPTS   4   /* 4 x 1GB = 4GB, matches the NPT's coverage */

/*
 * hvcore's own flat identity-mapped page tables, stored as static arrays
 * inside hvcore's loaded image — i.e. within [__hvcore_start, __hvcore_end),
 * which npt_build() already marks not-present in the guest's NPT view. The
 * guest can therefore never reach or corrupt these tables, unlike the 32bit
 * loader's page tables (CR3≈0xb1b000) hvcore otherwise keeps running on,
 * which sit in the same low physical memory the guest's own kernel/initrd
 * are loaded into (NPT is a flat 1:1 identity map). See
 * docs/SVM_HOST_PAGING_FIX.md for the full story.
 */
static pml4_t host_pml4[HOST_PT_NR_ENTRIES] __aligned_4k;
static pml3_t host_pdpt[HOST_PT_NR_ENTRIES] __aligned_4k;
static pml2_t host_pd[HOST_PT_NR_PDPTS][HOST_PT_NR_ENTRIES] __aligned_4k;

void cpu_init_host_paging(void)
{
    bzero(host_pml4, sizeof(host_pml4));
    bzero(host_pdpt, sizeof(host_pdpt));
    bzero(host_pd,   sizeof(host_pd));

    for (int g = 0; g < HOST_PT_NR_PDPTS; g++) {
        for (int i = 0; i < HOST_PT_NR_ENTRIES; i++) {
            uint64_t pa = ((uint64_t)g << PAGE_SHIFT_1G) | ((uint64_t)i << PAGE_SHIFT_2M);
            host_pd[g][i] = pa | PML_PRESENT | PML_RW | PML_SUPER | PML2_PS;
        }
        host_pdpt[g] = (uint64_t)host_pd[g] | PML_PRESENT | PML_RW | PML_SUPER;
    }
    host_pml4[0] = (uint64_t)host_pdpt | PML_PRESENT | PML_RW | PML_SUPER;

    cr3_write64((uint64_t)host_pml4);
}
