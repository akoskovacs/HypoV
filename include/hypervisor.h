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
struct Elf64_Image;

struct SystemInfo {
    struct MultiBootInfo    *s_boot_info;
    struct CharacterDisplay *s_display;
    struct CpuInfo          *s_cpu_info;
    struct PhysicalMMapping *s_phy_maps;
    struct Elf64_Image      *s_core_image;
    pml4_t                  *s_pml4;
};

void hv_entry(struct MultiBootInfo *);

#endif // HYPERVISOR_H
