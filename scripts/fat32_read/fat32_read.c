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
#include <stddef.h>

#include <fs/fat32.h>

#define JMP_INST   0xEB
#define JMP_OFFSET 0x58 /* Offset for the bootcode at the end (0x5A) */ 
#define NOP_INST   0x90

/* No writeback if debug is on */
bool is_debug_on = false;

struct FsReader {
    FILE *fs_file;
    const char *fs_file_name;
    const char *loader_file_name;
    struct Fat32_Sector0 *sector;
    uint8_t bootcode[FAT32_BOOTCODE_SIZE];
};

int fat32_dump(struct FsReader *hdl);

static void 
show_help(const char *prog_name)
{
    printf("Usage: %s -f filesystem\n", prog_name);
    printf("\t-f filesystem\t\tFAT32 filesystem\n");
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
fat32_read(struct FsReader *hdl, const char *fname)
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

#if 0
static bool
fat32_write_sector0(struct FsReader *hdl)
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
fat32_is_valid(struct FsReader *hdl)
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
    is_fat32 = (strncmp(fs_type, fat32_id, sizeof(fat32_id)) == 0)
            && ((sec->s_ext_boot.es_signature == FAT32_SIG0) 
            || (sec->s_ext_boot.es_signature == FAT32_SIG1));
    /* NOTE: The jump codes are not verified, but will be overwritten. */ 

    if (is_debug_on) {
        printf(is_fat32 ? "Seems to be a valid FAT32 filesystem :)\n" 
                        : "Invalid, corrupt FAT32 filesystem or something completely different :(\n");
    }

    return is_fat32;
}
#endif // 0

int fat32_dump(struct FsReader *hdl)
{
    struct Fat32_Sector0 *sec = hdl->sector;
    struct Fat32_BootSector *bs = &sec->s_boot;
    struct Fat32_ExtBootSector *es = &sec->s_ext_boot;
    char fs_type[10];
    char oem_name[10];
    char label[20];
    bool is_fat32;

    if (sec == NULL) {
        return false;
    }

    strncpy(fs_type, es->es_id, sizeof(fs_type)-1);
    fs_type[sizeof(es->es_id)] = '\0';

    strncpy(oem_name, bs->bs_oem_name, sizeof(oem_name)-1);
    strncpy(label, es->es_label, sizeof(label)-1);

    oem_name[sizeof(bs->bs_oem_name)] = '\0';
    label[sizeof(es->es_label)] = '\0';

    printf("Jump code: %#x %#x %#x\n", bs->bs_jump_code[0]
                                     , bs->bs_jump_code[1]
                                     , bs->bs_jump_code[2]);
    printf("OEM name: '%s' (offset: %#lx)\n", oem_name, offsetof(struct Fat32_BootSector, bs_oem_name));
    printf("FS ID: '%s' (offset: %#lx)\n", fs_type, offsetof(struct Fat32_BootSector, bs_oem_name));
    printf("Bytes per sector: %d (offset: %#lx)\n", bs->bs_bytes_per_sector, offsetof(struct Fat32_BootSector, bs_bytes_per_sector));
    printf("Sectors per cluster: %d (offset: %#lx)\n", bs->bs_sector_per_cluster, offsetof(struct Fat32_BootSector, bs_sector_per_cluster));
    printf("Number of reserved sectors: %d (offset: %#lx)\n", bs->bs_nr_reserved_sectors, offsetof(struct Fat32_BootSector, bs_nr_reserved_sectors));
    printf("Number of FATs: %d (offset: %#lx)\n", bs->bs_nr_fats, offsetof(struct Fat32_BootSector, bs_nr_fats));
    printf("Number of directory entries: %d (offset: %#lx)\n", bs->bs_nr_dir_entries, offsetof(struct Fat32_BootSector, bs_nr_dir_entries));
    printf("Number of sectors: %d (offset: %#lx)\n", bs->bs_nr_sectors, offsetof(struct Fat32_BootSector, bs_nr_sectors));
    printf("Media type: %#x (offset: %#lx)\n", bs->bs_media_type, offsetof(struct Fat32_BootSector, bs_media_type));
    printf("Sectors per FAT: %d (offset: %#lx)\n", bs->bs_sectors_per_fat, offsetof(struct Fat32_BootSector, bs_sectors_per_fat));
    printf("Sectors per track: %d (offset: %#lx)\n", bs->bs_sectors_per_track, offsetof(struct Fat32_BootSector, bs_sectors_per_fat));
    printf("Number of heads: %d (offset: %#lx)\n", bs->bs_nr_heads, offsetof(struct Fat32_BootSector, bs_nr_heads));
    printf("Number of hidden sectors: %d (offset: %#lx)\n", bs->bs_nr_hidden_sectors, offsetof(struct Fat32_BootSector, bs_nr_hidden_sectors));
    printf("Number of sectors [large]: %d (offset: %#lx)\n", bs->bs_nr_sectors_large, offsetof(struct Fat32_BootSector, bs_nr_sectors_large));
    printf("-------------------- Extended Boot Sector --------------------\n");
    printf("Sectors per FAT: %d (offset: %#lx)\n", es->es_sectors_per_fat
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_sectors_per_fat));
    printf("Flags: %#x (offset: %#lx)\n", es->es_flags
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_flags));
    printf("Version number: %#x (offset: %#lx)\n", es->es_vernum
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_vernum));
    printf("Number of root clusters: %d (offset: %#lx)\n", es->es_nr_root_clusters
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_nr_root_clusters));
    printf("Filesystem info cluster: %d (offset: %#lx)\n", es->es_fs_info_cluster
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_fs_info_cluster));
    printf("Backup boot sector: %d (offset: %#lx)\n", es->es_backup_boot_sector
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_backup_boot_sector));
    printf("Drive number: %#x (offset: %#lx)\n", es->es_drive_number
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_drive_number));
    printf("Windows NT flags: %#x (offset: %#lx)\n", es->es_win_nt_flags
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_win_nt_flags));
    printf("Signature: %#x (offset: %#lx)\n", es->es_signature
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_signature));
    printf("Serial: %#x (offset: %#lx)\n", es->es_serial
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_serial));
    printf("Label: '%s' (offset: %#lx)\n", label
        , sizeof(struct Fat32_BootSector) + offsetof(struct Fat32_ExtBootSector, es_label));

    return is_fat32;
    return 0;
}


int main(int argc, char * const argv[])
{
    int opt;
    struct FsReader hdl;
    bool has_fs = false;

    hdl.fs_file          = NULL;
    hdl.fs_file_name     = NULL;
    hdl.loader_file_name = NULL;
    hdl.sector           = NULL;

    while ((opt = getopt(argc, argv, "hdf:")) != -1) {
        switch (opt) {
            case 'h':
                show_help(argv[0]);
            return 0;

            case 'd':
                is_debug_on = true;
            break;

            case 'f':
                fat32_read(&hdl, optarg);
                has_fs = true;
            break;

            default:
            break;
        } 
    }

    if (!has_fs) {
        fprintf(stderr, "Please, specify the output filesystem/device with the '-f' option!\n");
        return 1;
    }

    return fat32_dump(&hdl);
}
