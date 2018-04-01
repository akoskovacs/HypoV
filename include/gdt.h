/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Global Descriptor Table bits (for C and assembly)          |
 * +------------------------------------------------------------+
*/

#ifndef GDT_H
#define GDT_H

#define DESC_32BIT          (1 << 22)
#define DESC_64BIT          (1 << 21)
#define DESC_PRESENT        (1 << 15)
#define DESC_CS_READ        (1 << 9)
#define DESC_DS_WRITE       (1 << 9)
#define DESC_DATA           (1 << 12)  /* bit 11 = 0 */
#define DESC_CODE           ((1 << 12) | (1 << 11))
#define DESC_TSS            ((1 << 8)  | (1 << 11))
#define DESC_GRANULAR       (1 << 23)
#define DESC_SEGLIMIT_HIGH  (0xF << 16)

#define GDT_NR_ENTRIES  10  /* Number of GDT entries for the container */
#define GDT_SZ_ENTRY    8   /* Size of each legacy entry */

#define GDT_LIMIT_MAX32 0xFFFFFFFF
#define GDT_LIMIT_MAX16 0xFFFF

#define GDT_SYS_NULL    0
#define GDT_SYS_CODE16  1
#define GDT_SYS_DATA16  2
#define GDT_SYS_CODE32  3
#define GDT_SYS_DATA32  4
#define GDT_SYS_CODE64  5
#define GDT_SYS_DATA64  6
#define GDT_SYS_TSS32   7
#define GDT_SYS_TSS64   8

#define GDT_NR_HV_ENTRIES 4/* Number of GDT entries for the hypervisor */
#define GDT_HV_NULL    0
#define GDT_HV_CODE64  1
#define GDT_HV_DATA64  2
#define GDT_HV_IDT64   3
#define GDT_HV_TSS64   3

#define TSS_BUSY        0xB00
#define TSS_AVAILABLE   0x900
#define TSS_GRANULAR    (1 << 23)

#define GDT_SEL(gdt_idx)    ((gdt_idx) * GDT_SZ_ENTRY)

#define GDT_CODE16_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ)
#define GDT_DATA16_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE)

#define GDT_CODE32_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_32BIT | DESC_GRANULAR)
#define GDT_DATA32_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE | DESC_32BIT | DESC_GRANULAR)

#define GDT_CODE64_FLAGS (DESC_PRESENT | DESC_CODE | DESC_CS_READ | DESC_64BIT)
#define GDT_DATA64_FLAGS (DESC_PRESENT | DESC_DATA | DESC_DS_WRITE | DESC_64BIT)

#define GDT_IDT64_FLAGS 0x0

#define CR0_PG_BIT 31

#endif // GDT_H