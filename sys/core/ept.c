/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | Extended Page Tables (EPT) — 1:1 guest-physical to         |
 * | host-physical mapping, 2MB pages, first 512MB of RAM       |
 * +------------------------------------------------------------+
 */
#include <memory.h>
#include <print.h>
#include <string.h>
#include <vmx.h>

extern struct CharacterDisplay debug_serial;

#define EPT_NR_ENTRIES 512
#define EPT_MAP_MB 512                        /* map first 512MB */
#define EPT_MAP_PAGES (EPT_MAP_MB * 1024 / 2) /* in 2MB pages = 256 */

static uint64_t ept_pml4[EPT_NR_ENTRIES] __aligned_4k;
static uint64_t ept_pdpt[EPT_NR_ENTRIES] __aligned_4k;
static uint64_t ept_pd[EPT_NR_ENTRIES] __aligned_4k;

uint64_t ept_build(void)
{
    bzero(ept_pml4, sizeof(ept_pml4));
    bzero(ept_pdpt, sizeof(ept_pdpt));
    bzero(ept_pd, sizeof(ept_pd));

    /* Map each 2MB page: guest-physical == host-physical (identity) */
    for (int i = 0; i < EPT_MAP_PAGES; i++)
    {
        uint64_t pa = (uint64_t)i << 21; /* i * 2MB */
        ept_pd[i] = pa | EPT_LARGE_PAGE | EPT_RWX | EPT_MEMTYPE_WB;
    }

    /* PDPT[0] → PD covering first 1GB */
    ept_pdpt[0] = (uint64_t)ept_pd | EPT_RWX;

    /* PML4[0] → PDPT covering first 512GB */
    ept_pml4[0] = (uint64_t)ept_pdpt | EPT_RWX;

    uint64_t eptp = (uint64_t)ept_pml4 | EPTP_MEMTYPE_WB | EPTP_PAGE_WALK_4;

    hv_printf(&debug_serial, "EPT: built, EPTP=%x, %dMB mapped\n", eptp, EPT_MAP_MB);
    return eptp;
}
