/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Very crude heap allocator                                  |
 * +------------------------------------------------------------+
*/

#include <basic.h>
#include <memory.h>

/* Additional mapping capacity */
#define MMAP_ADDITIONAL 2

/* The end of the loaded image, defined by the linker script */
extern uint32_t __image_end;

/* Heap end, initialized by the first heap expansion */
static uint8_t *current_heap_end = NULL;

/* Very crude heap allocator. The heap could only expand, 
   no free() is available. */
void *hv_expand_heap(size_t sz)
{
    uint8_t *ptr;

    /* Initialize the current heap pointer (done only once) */
    if (current_heap_end == NULL) {
        current_heap_end = (uint8_t *)&__image_end;
    }

    ptr = current_heap_end;
    current_heap_end += sz;
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
    phy_map = (struct PhysicalMMapping *)hv_expand_heap(sz_mapping);
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

struct PhysicalMMapping *hv_mm_init_mapping(struct MultiBootInfo *mbi) 
{
    struct PhysicalMMapping *phy_map = NULL;
    MMapEntry *mmap;
    struct MemoryMap *sm_map;
    int nr_map;

    if ((mbi == NULL) && (mbi->flags & MB_INFO_MEM_MAP)) {
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