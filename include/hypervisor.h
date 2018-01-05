/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor system functions                                |
 * +------------------------------------------------------------+
*/
#ifndef HYPERVISOR_H
#define HYPERVISOR_H

#include <types.h>

struct MultiBootInfo;
struct CharacterDisplay;
struct CpuInfo;
struct PhysicalMMapping;
struct MemoryMap;
struct Elf64_Image;

struct SystemInfo {
    struct MultiBootInfo    *s_boot_info;   /* Information from the bootloader */
    struct CharacterDisplay *s_display;     /* Current display */
    struct CpuInfo          *s_cpu_info;    /* Processor info */
    struct PhysicalMMapping *s_phy_maps;    /* All physical memory map (by BIOS) */
    struct Elf64_Image      *s_core_image;  /* ELF64 image of the hvcore */
    struct MemoryMap        *s_core_map;    /* Physical region of the hvcore */
    pml4_t                  *s_pml4;        /* Current system-wide page table */
};

void hv_entry(struct MultiBootInfo *);

#endif // HYPERVISOR_H
