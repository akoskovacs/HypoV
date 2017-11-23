/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Very crude heap allocator                                  |
 * +------------------------------------------------------------+
*/

#include <basic.h>
#include <memory.h>

/* The end of the loaded image, defined by the linker script */
extern void *__image_end;

/* Heap end, initialized by the first heap expansion */
void *current_heap_end = NULL;

/* Very crude heap allocator. The heap could only expand, 
   no free() is available. */
void *hv_expand_heap(size_t sz)
{
    void *ptr;

    /* Initialize the current heap pointer (done only once) */
    if (current_heap_end == NULL) {
        current_heap_end = __image_end;
    }

    ptr = current_heap_end;
    current_heap_end += sz;
    return ptr;
}