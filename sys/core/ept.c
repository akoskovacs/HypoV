/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | Extended Page Tables (EPT) — 1:1 guest-physical to         |
 * | host-physical mapping, 2MB pages, first 4GB               |
 * +------------------------------------------------------------+
 */
#include <memory.h>
#include <print.h>
#include <string.h>
#include <vmx.h>

extern struct CharacterDisplay debug_serial;

#define EPT_NR_ENTRIES  512
#define EPT_NR_PDPTS    4       /* 4 × 1GB = 4GB */

/* UC memory type for EPT leaf entries (bits 5:3 = 0) */
#define EPT_MEMTYPE_UC  (0ULL << 3)

static uint64_t ept_pml4[EPT_NR_ENTRIES] __aligned_4k;
static uint64_t ept_pdpt[EPT_NR_ENTRIES] __aligned_4k;
static uint64_t ept_pd[EPT_NR_PDPTS][EPT_NR_ENTRIES] __aligned_4k;

uint64_t ept_build(bool enable_ad)
{
    bzero(ept_pml4, sizeof(ept_pml4));
    bzero(ept_pdpt, sizeof(ept_pdpt));
    bzero(ept_pd,   sizeof(ept_pd));

    for (int g = 0; g < EPT_NR_PDPTS; g++) {
        for (int i = 0; i < EPT_NR_ENTRIES; i++) {
            uint64_t pa = ((uint64_t)g << 30) | ((uint64_t)i << 21);
            /* VGA framebuffer and legacy MMIO hole: uncached */
            uint64_t mt = (pa >= 0xA0000 && pa < 0x100000)
                          ? EPT_MEMTYPE_UC : EPT_MEMTYPE_WB;
            ept_pd[g][i] = pa | EPT_LARGE_PAGE | EPT_RWX | mt;
        }
        ept_pdpt[g] = (uint64_t)ept_pd[g] | EPT_RWX;
    }
    ept_pml4[0] = (uint64_t)ept_pdpt | EPT_RWX;

    uint64_t eptp = (uint64_t)ept_pml4 | EPTP_MEMTYPE_WB | EPTP_PAGE_WALK_4;
    if (enable_ad)
        eptp |= EPTP_ENABLE_AD;

    hv_printf(&debug_serial, "EPT: built, EPTP=%x, 4GB mapped\n", eptp);
    return eptp;
}
