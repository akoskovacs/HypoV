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

/* Physical bounds of HypoV's own loaded image (sys/core/hvcore.lds) */
extern char __hvcore_start[];
extern char __hvcore_end[];

/* Filled in by the loader before it jumps here — see struct HvBootHandoff
 * in memory.h and hv_start() in hvisor.c. May be NULL (e.g. CONFIG_HV_OS_STUB). */
extern struct HvBootHandoff *hv_boot_handoff;

#define NPT_LARGE_PAGE_SZ  0x200000ULL  /* 2MB, matches NPT_LARGE PDEs */

/* Mark every 2MB page *fully contained* in [start, end) not-present, so the
 * guest gets a reported NPF instead of silent read/write access. Addresses
 * are rounded INWARD to the enclosing 2MB pages (clamped to the 4GB the NPT
 * covers) — never outward.
 *
 * Rounding outward would hide whole 2MB pages that only partially overlap
 * [start, end), swallowing adjacent, perfectly ordinary RAM the guest
 * legitimately uses. That's not hypothetical: it's exactly what broke guest
 * boot twice — once for the loader's low-memory range (hid the guest's own
 * entry point at GPA=0x8000) and once for a small ~128KB MM_RESERVED sliver
 * at 0x1FFE0000 (outward-rounding pulled the hidden start down to 0x1FE00000,
 * eating ~1.9MB of real RAM and producing NPF GPA=0x1FFDFF28). Rounding
 * inward means small or oddly-aligned reserved regions may end up not hidden
 * at all — harmless, since the guest gets its own memory map and this NPT
 * hiding is only a coarse, best-effort defense-in-depth measure on top of
 * that. See docs/SVM_HOST_PAGING_FIX.md. */
static void npt_hide_range(uint64_t start, uint64_t end)
{
    uint64_t npt_limit = (uint64_t)NPT_NR_PDPTS << 30;
    if (end > npt_limit)
        end = npt_limit;
    if (start >= end)
        return;

    start = (start + NPT_LARGE_PAGE_SZ - 1) & ~(NPT_LARGE_PAGE_SZ - 1);
    end  &= ~(NPT_LARGE_PAGE_SZ - 1);
    if (start >= end)
        return;

    for (uint64_t pa = start; pa < end; pa += NPT_LARGE_PAGE_SZ) {
        int g = (int)(pa >> 30);
        int i = (int)((pa >> 21) & (NPT_NR_ENTRIES - 1));
        npt_pd[g][i] &= ~(uint64_t)NPT_PRESENT;
    }
}

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

    /* The flat identity map above gives the guest read/write access to all
     * of physical memory — including the hypervisor itself. Hide HypoV's own
     * hvcore image (this code, .data/.bss, NPT/VMCB/host-save, and now also
     * the host's own page tables — see cpu_init_host_paging() in
     * sys/core/host_cpu.c) so the guest can't read or corrupt it; any access
     * is reported as an NPF instead.
     *
     * NOTE: we deliberately do NOT hide the loader's low-memory footprint —
     * the guest's own boot code is placed by us at 0x8000 (see
     * svm_run_guest), squarely inside that low-memory region (confirmed:
     * hiding [0, low_mem_end) produced an immediate NPF at GPA=0x8000/
     * RIP=0x8000, the guest's own entry point). The guest and the 32bit
     * loader intentionally share that physical range; see
     * docs/SVM_HOST_PAGING_FIX.md for how host-page-table corruption is
     * instead solved by no longer depending on page tables that live there. */
    npt_hide_range((uint64_t)__hvcore_start, (uint64_t)__hvcore_end);

    /* Firmware/GRUB-reported MM_RESERVED ranges (ACPI tables, MMIO windows,
     * etc.) are never ordinary RAM — hide them too, so the guest can't
     * mistake them for usable memory. npt_hide_range() now rounds inward
     * (see its comment), so small or oddly-aligned reserved regions — e.g.
     * the EBDA or BIOS ROM, both well under 2MB and sharing a page with the
     * guest's own entry point at GPA=0x8000 — simply end up not hidden,
     * rather than dragging adjacent real RAM down with them. */
    int nr_hidden = 0;
    if (hv_boot_handoff != NULL) {
        for (uint64_t i = 0; i < hv_boot_handoff->nr_reserved; i++) {
            npt_hide_range(hv_boot_handoff->reserved_start[i],
                           hv_boot_handoff->reserved_end[i]);
            nr_hidden++;
        }
    }

    hv_printf(&debug_serial,
              "NPT: built, root=%x, 4GB mapped, hv image %x-%x hidden, "
              "%d reserved ranges hidden\n",
              npt_pml4, (uint64_t)__hvcore_start, (uint64_t)__hvcore_end,
              nr_hidden);
    return (uint64_t)npt_pml4;
}
