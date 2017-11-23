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

void *hv_expand_heap(size_t sz);

enum MEMORY_MAP_FLAGS {
    MM_FREE     = 0,
    MM_RESERVED = (1 << 1),
    MM_SELECTED = (1 << 2)
};

struct MemoryMap {
    uint64_t mm_start;
    uint64_t mm_end;
    uint32_t flags;
};

struct SystemMemory {
    int               sm_nr_maps;
    struct MemoryMap  sm_maps[];
};

#endif // MEMORY_H