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

#define NR_HV_GDT_ELEMS 5

static struct GDTEntry   gdt_table[NR_HV_GDT_ELEMS] __aligned_16;
static struct IDT64Entry idt_table[NR_HV_INTERRUPTS] __aligned_16;
static struct TSS64      tss_sys_64 __aligned_16;

void __gdt_setup_64(uint16_t seglimit, unsigned long base);
void __tss_setup_64(uint16_t tss_selector);
void __idt_setup_64(uint16_t seglimit, unsigned long base);

/* The full interrupt vector with auto-generated assembly wrappers */
extern hv_int_handler_ft __int_vector[];

void cpu_init_tables(void)
{
    //bzero((void *)&tss_sys_64, sizeof(tss_sys_64));
    gdt_make_entry(gdt_table + GDT_HV_NULL, 0x0, 0x0, 0x0);
    gdt_make_entry(gdt_table + GDT_HV_CODE64, 0x0, GDT_LIMIT_MAX32, GDT_CODE64_FLAGS);
    gdt_make_entry(gdt_table + GDT_HV_DATA64, 0x0, GDT_LIMIT_MAX32, GDT_DATA64_FLAGS);
    /* The size of the 64bit IDT entry is twice of the legacy ones (needs two entries) */
    gdt_make_entry(gdt_table + GDT_HV_IDT64, 0x0, GDT_LIMIT_MAX32, GDT_IDT64_FLAGS);
    gdt_make_entry(gdt_table + GDT_HV_IDT64 + 1, 0x0, GDT_LIMIT_MAX32, GDT_IDT64_FLAGS);
    /* Two entries for the 64 bit TSS */
    gdt_make_entry(gdt_table + GDT_HV_TSS64, (long)&tss_sys_64, sizeof(tss_sys_64), DESC_PRESENT | TSS_AVAILABLE);
    gdt_make_entry(gdt_table + GDT_HV_TSS64 + 1, 0x0, 0x0, 0x0);
    __gdt_setup_64((uint16_t)NR_HV_GDT_ELEMS * sizeof(gdt_table[0]) - 1, (unsigned long)&gdt_table);

    /* Set up interrupts */
    for (int i = 0; i < NR_HV_INTERRUPTS; i++) {
        idt64_make_entry(idt_table + i, true, GDT_HV_CODE64, __int_vector[i]);
    }
    bochs_breakpoint();

    __idt_setup_64((uint16_t)sizeof(idt_table) - 1, (unsigned long)&idt_table);
    int_enable();
}
