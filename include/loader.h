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

struct MemoryMap;

struct Elf64_Image
{
    void               *i_start;
    void               *i_end;

    struct Elf64_Hdr   *i_header;
    struct Elf64_Shdr  *i_sections;
    size_t              i_nr_sections;
    struct Elf64_Phdr  *i_phdrs;
    size_t              i_nr_phdrs;
    char               *i_strtab;        /* NULL terminated */
    size_t              i_strtab_size;

    void (*i_entry)(uint32_t);
};

struct SystemInfo;

bool elf64_is_header_valid(struct Elf64_Hdr *header);
struct Elf64_Image *elf64_load(void *image_begin, void *image_end, void *target, int *error);
struct Elf64_Image *ld_load_hvcore(struct MemoryMap *hvmap, int *error);
int                 ld_call_hvcore(struct SystemInfo *sysinfo);

#endif // LOADER_H