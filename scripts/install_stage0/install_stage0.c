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

#define BOOTCODE_SIZE 422

bool is_debug_on = false;

struct StageInstaller {
    FILE *fs_file;
    FILE *loader_image;
    const char *fs_file_name;
    const char *loader_file_name;
    struct Fat32_Sector0 *sector;
    uint8_t bootcode[BOOTCODE_SIZE];
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
    FILE *fp = open_file(fname, "rw");

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
fat32_is_valid(struct StageInstaller *hdl)
{
    struct Fat32_Sector0 *sec = hdl->sector;
    char fs_type[10];
    char oem_name[10];
    char label[20];
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

        printf("OEM name: %s\n", oem_name);
        printf("FS ID: %s\n", fs_type);
        printf("LABEL: %s\n", label);
    }
    return true;
}

static void 
loader_read(struct StageInstaller *hdl, const char *fname)
{
    size_t rs = 0;
    uint8_t sg0, sg1;
    FILE *fp = open_file(fname, "r");

    hdl->loader_image = fp;
    hdl->loader_file_name = fname;

    rs = fread(hdl->bootcode, sizeof(uint8_t), BOOTCODE_SIZE, fp);
    if (rs != BOOTCODE_SIZE) {
        fprintf(stderr, "Cannot read loader file '%s'\n", fname);
        exit(1);
    }

    sg0 = hdl->bootcode[BOOTCODE_SIZE-2];
    sg1 = hdl->bootcode[BOOTCODE_SIZE-1];
    if ((sg0 != 0x55) && sg1 != 0xAA) {
        fprintf(stderr, "Invalid stage0 loader signature got %#x, instead of 0x55AA!\n", (sg0 << 8) | sg1);
        exit(1);
    }
}

static int
install_loader(struct StageInstaller *hdl)
{
    fat32_is_valid(hdl);
    return 0;
}

int main(int argc, const char *argv[])
{
    int opt;
    struct StageInstaller hdl;
    bool has_fs, has_loader;

    has_fs = has_loader = false;

    hdl.fs_file = NULL;
    hdl.loader_image = NULL;
    hdl.fs_file_name = NULL;
    hdl.loader_file_name = NULL;
    hdl.sector = NULL;

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
