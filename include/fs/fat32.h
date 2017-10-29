/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | FAT32 on-disk and in-memoy datastructures                  |
 * +------------------------------------------------------------+
*/

#ifndef FAT32_FS_H
#define FAT32_FS_H

#ifdef __HVISOR__
# include <types.h>
#else
# include <inttypes.h>
#endif 

#include <basic.h>

struct __packed Fat32_ParameterBlock {

};

struct __packed Fat32_BootSector {
    uint8_t     bs_jump_code[3];        // jmp short 3C
    char        bs_oem_name[8];
    uint16_t    bs_bytes_per_sector;
    uint8_t     bs_sector_per_cluster;
    uint16_t    bs_nr_reserved_sectors;
    uint8_t     bs_nr_fats;
    uint16_t    bs_nr_dir_entries;
    uint16_t    bs_nr_sectors;          // 0 if more than 65536
    uint8_t     bs_media_type;
    uint16_t    bs_sectors_per_fat;     // FAT12/FAT16 only
    uint16_t    bs_sectors_per_track;
    uint16_t    bs_nr_heads;
    uint32_t    bs_nr_hidden_sectors;
    uint32_t    bs_nr_sectors_large;    // Used when bs_nr_sectors == 0
};

struct __packed Fat32_ExtBootSector {
    uint32_t    es_sectors_per_fat;
    uint16_t    es_flags;
    uint16_t    es_vernum;
    uint32_t    es_nr_root_clusters;
    uint16_t    es_fs_info_cluster;
    uint16_t    es_backup_boot_sector;
    uint8_t     es_reserved[12];
    uint8_t     es_drive_number;
    uint8_t     es_win_nt_flags;
    uint8_t     es_signature;          // 0x28 or 0x29
    uint32_t    es_serial;
    char        es_label[11];          // Padded with spaces
    char        es_id[8];              // 'FAT32 '
};

struct __packed Fat32_Sector0 {
    struct Fat32_BootSector    s_boot;
    struct Fat32_ExtBootSector s_ext_boot;
    uint8_t                    s_boot_code[422]; // The boot code, including the BIOS signature
};

struct Fat32_Fs {
    struct Fat32_Sector0 *fat_sector0;
};

#endif // FAT32_FS_H