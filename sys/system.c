/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Basic system functions (port input/output/etc)             |
 * +------------------------------------------------------------+
*/

#include <system.h>
#include <types.h>
#include <string.h>

int cpuid_get_branding(char branding[49])
{
    int32_t *bptr = (int32_t *)branding;
    cpuid(0x80000002, bptr);
    bptr += 4;
    cpuid(0x80000003, bptr);
    bptr += 4;
    cpuid(0x80000004, bptr);
    branding[48] = '\0';

    return 0;
}

int cpuid_get_vendor(char vendor[13])
{
    int32_t tmp_reg;
    int32_t vend[4];

    /* cpuid() function gives { EAX, EBX, ECX, EDX }, but the
       vendor id string is in { EBX, EDX, ECX } order */

    cpuid(0, vend);

    /* Swap EDX with ECX */
    tmp_reg             = vend[CPUID_REG_ECX];
    vend[CPUID_REG_ECX] = vend[CPUID_REG_EDX];
    vend[CPUID_REG_EDX] = tmp_reg;

    /* Skip EAX, it is not in the string */
    strncpy(vendor, (const char *)(vend+1), 12);
    vendor[12] = '\0';
    return 0;
}

/*
 * Disable i8259 Programmable Interrupt Controller. 
 * We will use APIC/x2APIC instead. 
*/
void pic_disable(void)
{
    outb(0xA0, 0xFF);
    outb(0x21, 0xFF);
}
