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
#include <hypervisor.h>
#include <system.h>
#include <print.h>
#include <xz.h>

#define ARCH_NOSPEC  0x0
#define ARCH_X86     0x3
#define ARCH_X86_64  0x3E
#define SZ_MAX_PAYLOAD 10*1024*1024 // 10MB

/* Pointers to the embedded ELF64 core image */
extern uint32_t __hvcore_start;
extern uint32_t __hvcore_end;
extern uint32_t __hvcore_size;

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
        && (header->e_type     == ET_EXEC)
        && (header->e_entry    != 0x0);
}

static inline struct Elf64_Image *elf64_init_image(struct Elf64_Image *im)
{
    im->i_entry    = NULL;
    im->i_header   = NULL;
    im->i_phdrs    = NULL;
    im->i_sections = NULL;
    im->i_strtab   = NULL;
    return im;
}

static struct Elf64_Image *elf64_alloc_image(void)
{
    struct Elf64_Image *im = NULL;
    im = mm_expand_heap(sizeof(*im));
    if (im == NULL) {
        return NULL;
    }

    elf64_init_image(im);
    return im;
}

static struct Elf64_Image *elf64_parse(struct Elf64_Image *im, void *image_begin, void *image_end, int *error)
{
    struct Elf64_Hdr *ehdr = NULL;

    ehdr = (struct Elf64_Hdr *)image_begin;
    if (!elf64_is_header_valid(ehdr)) {
        *error = -HV_ENOVALID;
        return NULL;
    }

    /* The ELF metadata will not be copied into the target, pointers are pointing in the source */
    im->i_start       = image_begin;
    im->i_end         = image_end;

    im->i_header      = ehdr;
    im->i_phdrs       = (struct Elf64_Phdr *)(((uint8_t *)image_begin) + ehdr->e_phoff);
    im->i_nr_phdrs    = ehdr->e_phnum;
    im->i_sections    = (struct Elf64_Shdr *)(((uint8_t *)image_begin) + ehdr->e_shoff);
    im->i_nr_sections = ehdr->e_shnum;

    struct Elf64_Shdr *sh_strtab = im->i_sections + ehdr->e_shstrndx;
    /* If the .strtab is not valid, why bother with the other stuff */
    if (sh_strtab->sh_type != SHT_STRTAB || sh_strtab->sh_offset == 0) {
        *error = -HV_ENOVALID;
        return NULL;
    }
    im->i_strtab      = ((char *)image_begin) + sh_strtab->sh_offset;
    return im;
}

struct Elf64_Image *elf64_load(void *image_begin, void *image_end, void *target, int *error)
{
    struct Elf64_Image *im = NULL;

    im = elf64_alloc_image();
    if (im == NULL) {
        *error = -HV_ENOMEM;
        return NULL;
    }

    elf64_parse(im, image_begin, image_end, error);
    if (*error != 0) {
        return NULL;
    }

    uint8_t *prog_ptr = target; 
    void *pexec       = NULL;
    /* The linker script will never load anything to 0x0, hopefully :D */
    off_t base_off    = 0;
    struct Elf64_Phdr *phdrs = im->i_phdrs;
        
    /* Load the code and data from the program headers */
    // TODO: Paging protection stuff
    for (int i = 0; i < im->i_nr_phdrs; i++) {
        /* Great a loadable segment */
        if (phdrs[i].p_type == PT_LOAD && phdrs[i].p_paddr != 0 && phdrs[i].p_offset != 0) {
            uint8_t *prog_src = ((uint8_t *)image_begin) + phdrs[i].p_offset;
            if (base_off == 0) {
                /* For the first loaded segment, we need its base 
                   to compute the others' loading address */
                base_off = phdrs[i].p_paddr;
            }

            if (phdrs[i].p_flags & (PF_X | PF_R)) {
                /* Executable, readable program segment, therefore its address is needed 
                   when entering into it, to execute it later  */
                pexec = prog_ptr;
            }
            /* This is the offset where we going */
            prog_ptr += phdrs[i].p_paddr - base_off;
            // hv_printf(stdout, "loaded to: %x\n", prog_ptr);
            /* Load it to the destination, after a cleanup */
            bzero(prog_ptr, phdrs[i].p_memsz);
            memcpy(prog_ptr, prog_src, phdrs[i].p_filesz);
        }
    }

    if (pexec == NULL) {
        *error = -HV_ENOVALID;
        return im;
    } else {
        /* Yay entry point */
        im->i_entry = (void(*)(uint32_t))pexec;
    }

    return im;
}

static void show_deflate_error(char *error)
{
    // TODO
}

int ld_deflate_hvcore(struct MemoryMap *hvmap, int *error, void **elf_start)
{
    /* These constants come from the linker when the compressed image
       is embedded into the linked object file (by the build system, via objcopy) */
    unsigned char *im_start = (void *)&__hvcore_start;
    unsigned char *im_end   = (void *)&__hvcore_end;
    int im_size             = (int *)&__hvcore_size;

    uint32_t load_addr      = 0x0;
    int sz_deflated         = 0;
    int xz_ret;

    if (hvmap == NULL || hvmap->mm_start == 0x0) {
        return -HV_BADARG;
    }

    /* Forcefully truncate size, cannot load anything above 4GB */
    //load_addr = (uint32_t)hvmap->mm_start;
    *elf_start = mm_expand_heap(SZ_MAX_PAYLOAD);
    /* Decompress the ELF64 image to the selected physical region */
    xz_ret = unxz(im_start, im_size, NULL, NULL, *elf_start, &sz_deflated, show_deflate_error);

    if (xz_ret != XZ_OK) {
        return -HV_ENOVALID;
    }

    return sz_deflated;
}

struct Elf64_Image *ld_load_hvcore(struct MemoryMap *hvmap, int *error, void *im_start, int sz_image)
{
    struct Elf64_Image *im = NULL;
    void *im_end   = (unsigned char *)im_start + sz_image;
    uint32_t laddr = 0; 

    /* Forcefully truncate size, cannot load anything above 4GB */
    laddr = (uint32_t)hvmap->mm_start;
    im = elf64_load(im_start, im_end, (void *)laddr, error);
    if (*error != 0 || im == NULL) {
        return NULL;
    }
    return im;
}

extern void __cpu_call_64(uint32_t addr, uint32_t arg0);
extern int cpu_gdt_init(void);

int ld_call_hvcore(struct SystemInfo *sysinfo)
{
    if (sysinfo == NULL || sysinfo->s_core_map == NULL || sysinfo->s_core_image == NULL 
        || sysinfo->s_core_image->i_entry == NULL) {
            return -HV_BADARG;
    }

    bochs_breakpoint();
    uint32_t sys_addr  = (uint32_t)sysinfo;
    uint32_t func_addr = (uint32_t)sysinfo->s_core_image->i_entry;
    //hv_printf(stdout, "Calling into 64bit at %x\n", func_addr);
    __cpu_call_64(func_addr, sys_addr);
    return 0;
}
