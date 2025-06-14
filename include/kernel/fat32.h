#ifndef __FAT32_H
#define __FAT32_H

#include "utils.h"

#define FAT32_MOUNT_POINT "/boot"
#define LFN_NAME_LEN 255

#define DIR_ENTRY_UNUSED 0xe5
#define DIR_ENTRY_UNUSED_JP 0x05
#define DIR_ENTRY_LAST_AND_UNUSED 0x00
#define SPACE 0x20

#define FAT_VAL_MASK 0x0fffffff
#define FAT_ENTRY_FREE 0x0

#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20
#define ATTR_LONG_NAME (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

struct __attribute__((packed)) l_dir_entry {
    uint8_t LDIR_Ord;
    char LDIR_Name1[10];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    char LDIR_Name2[12];
    uint16_t LDIR_FstClusLO;
    char LDIR_Name3[4];
};

struct __attribute__((packed)) s_dir_entry {
    char DIR_Name[8];
    char DIR_Name_Ext[3];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
};

void fat32_init();

#endif