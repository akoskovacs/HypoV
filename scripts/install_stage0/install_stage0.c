/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Installer for the Volume Boot Record for FAT32 filesystems |
 * +------------------------------------------------------------+
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <fs/fat32.h>

#define JMP_INST   0xEB
#define JMP_OFFSET 0x58 /* Offset for the bootcode at the end (0x5A) */ 
#define NOP_INST   0x90

/* No writeback if debug is on */
bool is_debug_on = false;

struct StageInstaller {
    FILE *fs_file;
    const char *fs_file_name;
    const char *loader_file_name;
    struct Fat32_Sector0 *sector;
    uint8_t bootcode[FAT32_BOOTCODE_SIZE];
};

static void 
show_help(const char *prog_name)
{
    printf("Usage: %s -l loader -o output_fs\n", prog_name);
    printf("\t-l loader\t\tStage 0 VBR loader image to install\n");
    printf("\t-o filesystem\t\tIntallation target FAT32 filesystem (device or regular file)\n\n");
}

static FILE *
open_file(const char *fname, const char *mode)
{
    FILE *fp = fopen(fname, mode);

    if (fp == NULL) {
        fprintf(stderr, "Cannot open file '%s'!\n", fname);
        exit(1);
    }

    return fp;
}

static void
fat32_read(struct StageInstaller *hdl, const char *fname)
{
    struct Fat32_Sector0 *sect = malloc(sizeof(struct Fat32_Sector0));
    size_t rs = 0;
    FILE *fp = open_file(fname, "r+");

    if ((hdl == NULL) || (sect == NULL)) {
        fprintf(stderr, "Memoy allocation error!\n");
        exit(1);
    }

    hdl->fs_file = fp;
    hdl->sector = sect;
    hdl->fs_file_name = fname;

    rs = fread(sect, sizeof(struct Fat32_Sector0), 1, fp);
    if (rs != 1) {
        fprintf(stderr, "Cannot read file '%s'!\n", fname);
        exit(1);
    }
}

static bool
fat32_write_sector0(struct StageInstaller *hdl)
{
    struct Fat32_Sector0 *sect = hdl->sector;
    FILE *fsfile = hdl->fs_file;

    if ((sect == NULL) || (fsfile == NULL)) {
        return false;
    }

    if (fseek(fsfile, 0L, SEEK_SET) != 0L) {
        fprintf(stderr, "Can't seek back on '%s'!\n", hdl->fs_file_name);
        return false;
    }

    /* No writeback, but no problem either */
    if (is_debug_on) {
        return true;
    }

    /* TODO: Write the backup sector, too */
    if ((fwrite(sect, sizeof(struct Fat32_Sector0), 1, fsfile)) != 1) {
        fprintf(stderr, "Can't write filesystem on '%s'!\n", hdl->fs_file_name);
        return false;
    }
    return true;
}

static bool
fat32_is_valid(struct StageInstaller *hdl)
{
    struct Fat32_Sector0 *sec = hdl->sector;
    char fs_type[10];
    char oem_name[10];
    char label[20];
    char const fat32_id[] = "FAT32 ";
    bool is_fat32;

    if (sec == NULL) {
        return false;
    }

    strncpy(fs_type, sec->s_ext_boot.es_id, sizeof(fs_type)-1);
    fs_type[sizeof(sec->s_ext_boot.es_id)] = '\0';

    /* Dump the OEM name and label just for debugging purposes */
    if (is_debug_on) {
        strncpy(oem_name, sec->s_boot.bs_oem_name, sizeof(oem_name)-1);
        strncpy(label, sec->s_ext_boot.es_label, sizeof(label)-1);

        oem_name[sizeof(sec->s_boot.bs_oem_name)] = '\0';
        label[sizeof(sec->s_ext_boot.es_label)] = '\0';

        printf("Jump code: %#x %#x %#x\n", sec->s_boot.bs_jump_code[0]
                                         , sec->s_boot.bs_jump_code[1]
                                         , sec->s_boot.bs_jump_code[2]);
        printf("OEM name: %s\n", oem_name);
        printf("FS ID: %s\n", fs_type);
        printf("Label: %s\n", label);
    }

    /* Some say, you can't depend on the FAT32 ID string, but I still don't
        really like if it's not what we expect */
    is_fat32 = strncmp(fs_type, fat32_id, sizeof(fat32_id)) == 0;
    is_fat32 = is_fat32 && (sec->s_ext_boot.es_signature == FAT32_SIG0) 
                        || (sec->s_ext_boot.es_signature == FAT32_SIG1);
    /* NOTE: The jump codes are not verified, but will be overwritten. */ 

    if (is_debug_on) {
        printf(is_fat32 ? "Seems to be a valid FAT32 filesystem :)\n" 
                        : "Invalid, corrupt FAT32 filesystem or something completely different :(\n");
    }

    return is_fat32;
}

static void 
loader_read(struct StageInstaller *hdl, const char *fname)
{
    size_t rs = 0;
    uint8_t sg0, sg1;
    FILE *fp = open_file(fname, "r");

    hdl->loader_file_name = fname;

    rs = fread(hdl->bootcode, sizeof(uint8_t), FAT32_BOOTCODE_SIZE, fp);
    if (rs != FAT32_BOOTCODE_SIZE) {
        fprintf(stderr, "Cannot read loader file '%s'\n", fname);
        exit(1);
    }

    sg0 = hdl->bootcode[FAT32_BOOTCODE_SIZE-2];
    sg1 = hdl->bootcode[FAT32_BOOTCODE_SIZE-1];
    if ((sg0 != 0x55) && (sg1 != 0xAA)) {
        fprintf(stderr, "Invalid stage0 loader signature got %#x, instead of 0x55AA!\n", (sg0 << 8) | sg1);
        exit(1);
    }
    fclose(fp);
}

static int
install_loader(struct StageInstaller *hdl)
{
    bool success = false;
    struct Fat32_Sector0 *sect = hdl->sector;

    if (fat32_is_valid(hdl)) {
        /* The real deal */    
        /* Installing the new boot code */
        memcpy(sect->s_boot_code, hdl->bootcode, sizeof(hdl->bootcode));

        /* Installing the new jump to it (anyway) */
        sect->s_boot.bs_jump_code[0] = JMP_INST;
        sect->s_boot.bs_jump_code[1] = JMP_OFFSET;
        sect->s_boot.bs_jump_code[2] = NOP_INST;

        success = fat32_write_sector0(hdl);

        fclose(hdl->fs_file);
        free(hdl->sector);
        hdl->sector = NULL;
    } else {
        fprintf(stderr, "Invalid FAT32 filesystem. Use -d, to find out why...\n");
        return 1;
    }
    return success == false; /* Mind == blown */
}

int main(int argc, const char *argv[])
{
    int opt;
    struct StageInstaller hdl;
    bool has_fs, has_loader;

    has_fs = has_loader  = false;

    hdl.fs_file          = NULL;
    hdl.fs_file_name     = NULL;
    hdl.loader_file_name = NULL;
    hdl.sector           = NULL;

    while ((opt = getopt(argc, argv, "hdl:i:o:")) != -1) {
        switch (opt) {
            case 'h':
                show_help(argv[0]);
            return 0;

            case 'd':
                is_debug_on = true;
            break;

            case 'i': case 'o':
                fat32_read(&hdl, optarg);
                has_fs = true;
            break;

            case 'l':
                loader_read(&hdl, optarg);
                has_loader = true;
            break;

            default:
            break;
        } 
    }

    if (!has_loader) {
        fprintf(stderr, "Please, specify the loader file with the '-l' option!\n");
    }

    if (!has_fs) {
        fprintf(stderr, "Please, specify the output filesystem/device with the '-o' option!\n");
    }

    if (!has_fs || !has_loader) {
        return 1;
    }

    return install_loader(&hdl);
}
