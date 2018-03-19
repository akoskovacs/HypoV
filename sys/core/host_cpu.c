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

#define NR_HV_INTERRUPTS 256
#define NR_GDT_HV_ELEMS 5

typedef void (*hv_int_handler_ft)(void);

static struct GDTEntry gdt_table[NR_GDT_HV_ELEMS] __aligned_16;

static struct TSS64    tss_sys_64 __aligned_16;
static hv_int_handler_ft int_handlers[NR_HV_INTERRUPTS];

void __gdt_setup_64(uint16_t seglimit, unsigned long base);
void __tss_setup_64(uint16_t tss_selector);
void __idt_setup_64(unsigned long base);

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
    __gdt_setup_64(GDT_NR_HV_ENTRIES * sizeof(gdt_table[0]), (unsigned long)&gdt_table);
    __idt_setup_64((unsigned long)int_handlers);
    int_enable();
}
