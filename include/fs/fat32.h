/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | FAT32 on-disk and in-memoy datastructures                  |
 * +------------------------------------------------------------+
*/

#ifndef FAT32_FS_H
#define FAT32_FS_H

#include <types.h>

struct __packed Fat32_ParameterBlock {

};

struct __packed Fat32_BootSector {
    uint8_t bs_jump_code[3];
    uint8_t bs_oem_name[8];
    struct Fat32_ParameterBlock bs_param_block;
};


struct Fat32_Fs {
    struct Fat32_BootSector *ff_boot_sector;
};

#endif // FAT32_FS_H