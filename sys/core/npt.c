/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2026                           |
 * |                                                            |
 * | AMD Nested Page Tables (NPT) — 1:1 identity mapping        |
 * | Uses standard x86 page table format (unlike EPT)           |
 * +------------------------------------------------------------+
*/
#include <memory.h>
#include <print.h>
#include <string.h>
#include <svm.h>

extern struct CharacterDisplay debug_serial;

#define NPT_NR_ENTRIES  512
#define NPT_NR_PDPTS    4       /* 4 × 1GB = 4GB */

/* Standard x86 page table flags */
#define NPT_PRESENT  (1ULL << 0)
#define NPT_WRITABLE (1ULL << 1)
#define NPT_USER     (1ULL << 2)
#define NPT_LARGE    (1ULL << 7)  /* 2MB page in PDE */
#define NPT_RWX      (NPT_PRESENT | NPT_WRITABLE | NPT_USER)

static uint64_t npt_pml4[NPT_NR_ENTRIES] __aligned_4k;
static uint64_t npt_pdpt[NPT_NR_ENTRIES] __aligned_4k;
static uint64_t npt_pd[NPT_NR_PDPTS][NPT_NR_ENTRIES] __aligned_4k;

uint64_t npt_build(void)
{
    bzero(npt_pml4, sizeof(npt_pml4));
    bzero(npt_pdpt, sizeof(npt_pdpt));
    bzero(npt_pd,   sizeof(npt_pd));

    for (int g = 0; g < NPT_NR_PDPTS; g++) {
        for (int i = 0; i < NPT_NR_ENTRIES; i++) {
            uint64_t pa = ((uint64_t)g << 30) | ((uint64_t)i << 21);
            npt_pd[g][i] = pa | NPT_LARGE | NPT_RWX;
        }
        npt_pdpt[g] = (uint64_t)npt_pd[g] | NPT_RWX;
    }
    npt_pml4[0] = (uint64_t)npt_pdpt | NPT_RWX;

    hv_printf(&debug_serial, "NPT: built, root=%x, 4GB mapped\n", npt_pml4);
    return (uint64_t)npt_pml4;
}
