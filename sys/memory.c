/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Very crude heap allocator                                  |
 * +------------------------------------------------------------+
*/

#include <basic.h>
#include <error.h>
#include <memory.h>
#include <hypervisor.h>
#include <cpu.h>
#include <system.h>

/* Additional mapping capacity */
#define MMAP_ADDITIONAL 2

/* The end of the loaded image, defined by the linker script */
extern uint32_t __image_end;

/* Heap end, initialized by the first heap expansion */
static uint8_t *current_heap_end = NULL;

uint8_t *mm_get_heap_end(void)
{
    /* Initialize the current heap pointer (done only once) */
    if (current_heap_end == NULL) {
        current_heap_end = (uint8_t *)&__image_end;
    }

    return current_heap_end;
}

/* Very crude heap allocator. The heap could only expand, 
   no free() is available. */
void *mm_expand_heap(size_t sz)
{
    uint8_t *ptr = mm_get_heap_end();

    current_heap_end += sz;
    return (void *)ptr;
}

/*
 * Get a 4K aligned frame address
 * @nr_pages is the number of pages needed
*/
void *mm_expand_heap_4k(size_t nr_pages)
{
    uint8_t *ptr = mm_get_heap_end();

    /* Align the current end (XXX: Could be lower!) */
    unsigned long aligned = ((unsigned long)ptr) & PAGE_ALIGN_MASK_4K;

    /* We are really lucky if the end is already on a 4K boundary,
       but if it's not, get the next frame's address */
    if (aligned != (unsigned long)ptr) {
        /* Get the next 4K address */
        aligned += PAGE_SIZE_4K;
    }

    ptr = (uint8_t *)aligned;

    current_heap_end = ptr + (nr_pages * PAGE_SIZE_4K);
    return (void *)ptr;
}

typedef struct MultiBootMMapEntry MMapEntry;

/* Allocate space for the physical mapping information */
static struct PhysicalMMapping *alloc_physical_mapping(int nr_map)
{
    size_t sz_mapping;
    struct PhysicalMMapping *phy_map = NULL;

    /* Allocate the main structure and the array elements plus some additional */
    sz_mapping = sizeof(*phy_map) + ((nr_map + MMAP_ADDITIONAL) * sizeof(struct MemoryMap));
    phy_map = (struct PhysicalMMapping *)mm_expand_heap(sz_mapping);
    phy_map->sm_nr_alloc = nr_map + MMAP_ADDITIONAL;
    phy_map->sm_nr_maps  = nr_map;
    return phy_map;
}

/* Count the system mappings for allocation */
static int mb_get_mmap_nr_entries(struct MultiBootInfo *mbi)
{
    int nr_map      = 0;
    MMapEntry *mmap = (MMapEntry *)mbi->mmap_addr;
    while (mmap < (MMapEntry *)(mbi->mmap_addr + mbi->mmap_length)) {
        if (mmap->addr != 0 && mmap->length != 0) {
            nr_map++;
        }
        mmap = (MMapEntry *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }
    return nr_map;
}

struct PhysicalMMapping *mm_init_mapping(struct MultiBootInfo *mbi) 
{
    struct PhysicalMMapping *phy_map = NULL;
    MMapEntry *mmap;
    struct MemoryMap *sm_map;
    int nr_map;

    if (!mbi || !(mbi->flags & MB_INFO_MEM_MAP)) {
        return NULL;
    }
    nr_map  = mb_get_mmap_nr_entries(mbi);
    phy_map = alloc_physical_mapping(nr_map);

    /* Copy the the memory mapping got from the bootloader to our new structures */
    mmap   = (MMapEntry *)mbi->mmap_addr;
    sm_map = phy_map->sm_maps;
    nr_map = 0;
    while (mmap < (MMapEntry *)(mbi->mmap_addr + mbi->mmap_length)) {
        if (mmap->addr != 0 && mmap->length != 0) {
            sm_map->mm_start = mmap->addr;
            sm_map->mm_end   = mmap->addr + mmap->length;
            sm_map->mm_flags = (mmap->type == MB_MEMORY_RESERVED) ? MM_RESERVED : 0;
            sm_map++;
            nr_map++;
        }
        mmap = (MMapEntry *)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
    }

    /* Zero-out empty space */
    while (nr_map < phy_map->sm_nr_alloc) {
        phy_map->sm_maps[nr_map].mm_start   = 0L;
        phy_map->sm_maps[nr_map].mm_end     = 0L;
        phy_map->sm_maps[nr_map].mm_flags   = MM_EMPTY;
        nr_map++;
    }

    return phy_map;
}

/* Build the last level of identity pages */
static npa_t mm_init_pml2(pml2_t *pte, npa_t pa)
{
    for (int i = 0; i < PAGE_TBL_NR_ENTRIES; i++) {
        pte[i] = PML_PRESENT | PML_RW | PML_SUPER | PML2_PS | pa;
        pa    += PAGE_SIZE_2MB;
    }
    return (npa_t)pte;
}

static npa_t mm_init_pml3(void)
{
    /* Page for the third level entry pointers */
    pml3_t *pml3 = (pml3_t *)mm_expand_heap_4k(1);

    /* Page for all the entries themselves */
    pml2_t *pte  = (pml2_t *)mm_expand_heap_4k(PAGE_TBL_NR_ENTRIES);
    pml2_t *ptep = pte;

    /* Starting from physical address 0x0 */
    npa_t pa = 0x0;
    for (int i = 1; i < PAGE_TBL_NR_ENTRIES; i++) {
        npa_t pml2_pa = mm_init_pml2(ptep, pa);
        pml3[i]       = PML_PRESENT | PML_RW | PML_SUPER | pml2_pa;
        pa           += (PAGE_SIZE_2MB * PAGE_TBL_NR_ENTRIES);
        /* Get the next PML2 table */
        ptep          = (pml2_t *)((char *)ptep) + (PAGE_TBL_NR_ENTRIES * PAGE_SIZE_4K);
    }
    return (npa_t)pml3;
}

/*
 * Build up the identity page tables for the first 1GB.
 * Only the first entry is present for now.
*/
npa_t mm_init_pml4(pml4_t *pml4)
{
    int i;
    npa_t pde_pa = mm_init_pml3();
    pml4[0] = PML_PRESENT | PML_RW | PML_SUPER | pde_pa;

    /* PML_PRESENT is clear, so not used */
    for (i = 1; i < PAGE_TBL_NR_ENTRIES; i++) {
        pml4[i] = 0;
    }
    return (npa_t)pml4;
}

/*
 * Map the first entire 1GB physical memory into consencutive
 * 2MB pages starting from 0x0 (identity paging). Identity paging
 * mandatory for switching into 64bit mode.
*/
pml4_t *mm_init_page_tables(void)
{
    pml4_t *pml4_tbl = (pml4_t *)mm_expand_heap_4k(1);
    /* Only the first entry will be used at the beggining */
    mm_init_pml4(pml4_tbl);
    return pml4_tbl;
}

int mm_init_paging(struct SystemInfo *info)
{
    if (info == NULL || info->s_cpu_info == NULL) {
        return -HV_ENOINFO;
    }

    /* Needs PAE */ 
    if (!(info->s_cpu_info->ci_features & CPU_FEATURE_PAE)) {
        return -HV_ENOSUPP;
    }
    
    pml4_t *pl4 = mm_init_page_tables();
    if (pl4 == NULL) {
        return -HV_ENOMEM;
    }

    info->s_pml4 = pl4;

    /* 
     * Still in 32bit mode, all registers (including the controls)
     * are this length. 
     * This also means that our page table pointer must  be under 4GB,
     * which is fine for now.
     */

    /* Set up Physical Address Extensions */
    uint32_t cr4 = cr4_read() | CR4_PAE;
    cr4_write(cr4);

    /* Mr. Processor, please use our new page tables... */
    cr3_write((uint32_t)pl4);

    return 0;
}