#ifndef FAT32_H
#define FAT32_H

#include <stdint.h>
#include <stdbool.h>

/* FAT32 structures */
typedef struct {
    uint8_t  jump[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_dir_entries;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t flags;
    uint16_t version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     filesystem_type[8];
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    char     name[11];
    uint8_t  attributes;
    uint8_t  reserved;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

/* File attributes */
#define FAT_ATTR_READ_ONLY  0x01
#define FAT_ATTR_HIDDEN     0x02
#define FAT_ATTR_SYSTEM     0x04
#define FAT_ATTR_VOLUME_ID  0x08
#define FAT_ATTR_DIRECTORY  0x10
#define FAT_ATTR_ARCHIVE    0x20
#define FAT_ATTR_LONG_NAME  0x0F

/* FAT32 functions */
bool fat32_init(void);
bool fat32_read_file(const char *path, uint8_t *buffer, uint32_t *size);
bool fat32_write_file(const char *path, const uint8_t *buffer, uint32_t size);
bool fat32_list_directory(const char *path, fat32_dir_entry_t *entries, uint32_t *count);
bool fat32_file_exists(const char *path);
bool fat32_create_file(const char *path);
bool fat32_delete_file(const char *path);
uint32_t fat32_get_file_size(const char *path);

#endif /* FAT32_H */
