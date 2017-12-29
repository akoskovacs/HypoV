/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor ELF loader                                      |
 * +------------------------------------------------------------+
*/

#ifndef LOADER_H
#define LOADER_H

#include <elf.h>

struct Elf64_Image
{
    struct Elf64_Hdr *i_header;

    void (*i_entry)(void);
};

bool elf64_is_header_valid(struct Elf64_Hdr *header);
struct Elf64_Image *elf64_load(void *image_begin, pa_t target, int *error);

#endif // LOADER_H