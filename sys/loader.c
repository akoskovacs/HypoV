/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor ELF loader, used to load the final 64bit ELF64  |
 * | code to the memory                                         |
 * +------------------------------------------------------------+
*/

#include <error.h>
#include <loader.h>
#include <memory.h>

struct Elf64_Image *elf64_load(void *image_begin, pa_t i_target, int *error)
{
    if (error == NULL) {
        *error = -HV_ENOIMPL;
    }

    return NULL;
}