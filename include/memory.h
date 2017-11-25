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

void                    *hv_expand_heap(size_t sz);
struct PhysicalMMapping *hv_mm_init_mapping(struct MultiBootInfo *mbi);

#endif // MEMORY_H