/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Memory management stuff                                    |
 * +------------------------------------------------------------+
*/

#ifndef MEMORY_H
#define MEMORY_H

#include <types.h>
#include <boot/multiboot.h>

#define PAGE_SIZE_4K       0x00001000   // 4096
#define PAGE_SHIFT_4K      12
#define PAGE_SIZE_2M       0x00200000 // 1 << 21
#define PAGE_SHIFT_2M      21
#define PAGE_SHIFT_1G      30
#define PAGE_ALIGN_MASK_4K 0xfffff000
#define PAGE_ALIGN_MASK_2M 0xfff00000
#define PAGE_SIZE          PAGE_SIZE_2M

#define PML_PRESENT        (1 << 0)  // Page present
#define PML_RW             (1 << 1)  // Read/Write
#define PML_SUPER          (1 << 2)  // User/Supervisor
#define PML_PWT            (1 << 3)  // Page-level write-through
#define PML_PCD            (1 << 4)  // Page-level cache disable
#define PML_ACCESS         (1 << 5)
#define PML2_DIRTY         (1 << 6)
#define PML2_PS            (1 << 7)  // Page size, must be set for PML2
#define PML2_GLOBAL        (1 << 8)
// (9..11) Available to software
#define PML2_PAT           (1 << 12) // Page Attribute Table
#define PML_NX             (1 << 63) // No-execute

#define PAGE_TBL_NR_ENTRIES 512
/* Mask for the level index */
#define VA_PML_MASK         0x1FF    // 9 bits

#define VA_PML4_SHIFT       39
#define VA_PML3_SHIFT       30
#define VA_PML2_SHIFT       21
#define VA_PAGE_OFFSET_MASK 0x1FFFFF // Last 21 bits

/* Get table indexes from the virtual address */
#define VA_PML4_IDX(va) (((va_t)va) >> VA_PML4_SHIFT)
#define VA_PML3_IDX(va) ((((va_t)va) >> VA_PML3_SHIFT) & VA_PML_MASK)
#define VA_PML2_IDX(va) ((((va_t)va) >> VA_PML2_SHIFT) & VA_PAGE_OFFSET_MASK)

/* Mask for the table physical addresses (already aligned) */
#define PML_BASE_MASK   0xFFFFFFFFFF000ULL // Bits (51..12)
#define PAGE_MASK_2M    0xFFFFFFFE00000ULL // Bits (51..21)

/* XXX: All physical base addresses has to be 4K aligned */
#define VA_PML3_IDX(va) ((((va_t)va) >> VA_PML3_SHIFT) & VA_PML_MASK)
#define VA_PML2_IDX(va) ((((va_t)va) >> VA_PML2_SHIFT) & VA_PAGE_OFFSET_MASK)

enum MEMORY_MAP_FLAGS {
    MM_RESERVED = (1 << 1), /* Reserved by the hardware     */
    MM_SELECTED = (1 << 2), /* Selected for the hypervisor  */
    MM_EMPTY    = (1 << 3)  /* Filled in later              */
};

struct MemoryMap {
    uint64_t mm_start;
    uint64_t mm_end;
    uint32_t mm_flags;
};

/* 
 * These structures copied from the multiboot memory-mapping information
 * provider by the bootloader. Unfortunately the default format is not
 * designed for modification. We have to copy it to our heap to be able
 * to make them easier to work with.
*/
struct PhysicalMMapping {
    int               sm_nr_maps;   /* Number of mappings in sm_maps[] */
    int               sm_nr_alloc;  /* Full capacity of sm_maps[]      */
    struct MemoryMap  sm_maps[];
};

#define HV_HANDOFF_MAX_RESERVED 32

/*
 * Plain-old-data handoff from the 32bit loader to the 64bit hvcore payload.
 * struct SystemInfo can't be shared across that boundary as-is (its pointer
 * members are 4 bytes wide in the loader and 8 in hvcore), so this carries
 * only fixed-width integers — giving it an identical layout under both ABIs.
 * It lets hvcore build an NPT that actually hides the loader's own memory
 * (image, heap, host page tables) and the BIOS/firmware-reserved regions
 * GRUB reported, instead of presenting them to the guest as free RAM.
 */
struct HvBootHandoff {
    uint64_t low_mem_end;       /* end of the loader's own footprint (image+heap+page tables) */
    uint64_t nr_reserved;       /* number of valid entries below                              */
    uint64_t reserved_start[HV_HANDOFF_MAX_RESERVED];
    uint64_t reserved_end[HV_HANDOFF_MAX_RESERVED];
};

struct SystemInfo;

uint8_t                 *mm_get_heap_end(void);
void                    *mm_expand_heap(size_t sz);
void                    *mm_expand_heap_4k(size_t sz); // 4K aligned

struct PhysicalMMapping *mm_init_mapping(struct MultiBootInfo *mbi);
int                      mm_init_paging(struct SystemInfo *info);
pml4_t                  *mm_init_page_tables(void);

/* Snapshot the loader's own memory footprint and the firmware-reserved
 * regions from `info` into the ABI-safe `out`, ready to be handed over
 * to hvcore (see struct HvBootHandoff). Call only once mm_init_paging()
 * has finished — its page tables are the last thing the loader allocates,
 * so mm_get_heap_end() only reflects everything we own at that point. */
void mm_build_boot_handoff(struct SystemInfo *info, struct HvBootHandoff *out);

struct MemoryMap *mm_alloc_phymap(struct PhysicalMMapping *maps, unsigned int nr_pages, int *error);

// pa_t                     mm_virt_to_phys32(va_t);
// va_t                     mm_phys_to_virt32(pa_t);

#endif // MEMORY_H