/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor ELF loader, used to load the final 64bit ELF64  |
 * | code to the memory. The resulting image have to be free of |
 * | external dependencies, because those will not be resolved. |
 * +------------------------------------------------------------+
*/

#include <error.h>
#include <loader.h>
#include <memory.h>
#include <string.h>

#define ARCH_NOSPEC  0x0
#define ARCH_X86     0x3
#define ARCH_X86_64  0x3E

/*
 * Only 64bit, relocatable, SYSV-compatible little-endian
 * valid ELF64 binaries are good for us.
*/
bool elf64_is_header_valid(struct Elf64_Hdr *header)
{
    if (header == NULL) {
        return false;
    }

    return (header->e_ident[EI_MAG0]  == ELFMAG0)
        && (header->e_ident[EI_MAG1]  == ELFMAG1)
        && (header->e_ident[EI_MAG2]  == ELFMAG2)
        && (header->e_ident[EI_MAG3]  == ELFMAG3)
        && (header->e_ident[EI_CLASS] == ELFCLASS64)
        && (header->e_ident[EI_DATA]  == ELFDATA2LSB)
        && (header->e_version  == EV_CURRENT)
        && (header->e_type     == ET_DYN)
        && (header->e_machine  == ELF_OSABI);
}

struct Elf64_Image *elf64_load(void *image_begin, pa_t i_target, int *error)
{
    struct Elf64_Image *im = NULL;
    struct Elf64_Hdr *header = NULL;
    if (!error) {
        return NULL;
    }

    header = (struct Elf64_Hdr *)image_begin;
    if (!elf64_is_header_valid(header)) {
        *error = -HV_ENOVALID;
        return NULL;
    }

    im = mm_expand_heap(sizeof(*im));
    if (im == NULL) {
        *error = -HV_ENOMEM;
        return NULL;
    }

    header = mm_expand_heap(sizeof(*header));
    im->i_header = header;
    //im->i_entry  = (void(*)(void))header->e_entry;

    return im;
}