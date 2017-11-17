/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Basic system functions (port input/output/etc)             |
 * +------------------------------------------------------------+
*/

#include <system.h>
#include <types.h>

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
    int32_t tmp, i;
    /* cpuid() function gives { EAX, EBX, ECX, EDX }, but the
       vendor id is { EBX, EDX, ECX } */
    int32_t *vend[4];
    cpuid(0, vend);
    tmp = vend[3];       // save EDX
    vend[3] = vend[2];   // overwrite EDX with ECX
    vend[2] = tmp;       // and vica versa
    for (i = 0; i < 13; i++) {
        vendor[i] = *(((char *)vend)+i+4); // (+4) because EAX is uneeded
    }
    vendor[12] = '\0';
    return 0;
}