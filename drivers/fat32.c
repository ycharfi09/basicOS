#include "fat32.h"
#include "ata.h"
#include "../../kernel/include/memory.h"
#include <stdint.h>
#include <stdbool.h>

/* FAT32 state */
static fat32_boot_sector_t boot_sector;
static uint32_t fat_begin_lba = 0;
static uint32_t cluster_begin_lba = 0;
static uint32_t sectors_per_cluster = 0;
static uint32_t root_dir_first_cluster = 0;
static bool fs_initialized = false;

/* String helpers */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

/* Convert FAT32 name to 8.3 format */
static void convert_to_fat_name(const char *filename, char *fat_name) {
    memset(fat_name, ' ', 11);
    
    int i = 0, j = 0;
    while (filename[i] && filename[i] != '.' && j < 8) {
        fat_name[j++] = filename[i++];
    }
    
    if (filename[i] == '.') {
        i++;
        j = 8;
        while (filename[i] && j < 11) {
            fat_name[j++] = filename[i++];
        }
    }
}

/* Get LBA of a cluster */
static uint32_t cluster_to_lba(uint32_t cluster) {
    return cluster_begin_lba + (cluster - 2) * sectors_per_cluster;
}

/* Read FAT entry */
static uint32_t read_fat_entry(uint32_t cluster) {
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fat_begin_lba + (fat_offset / 512);
    uint32_t entry_offset = fat_offset % 512;
    
    uint8_t buffer[512];
    if (!ata_read_sectors(fat_sector, 1, buffer)) {
        return 0;
    }
    
    uint32_t *entry = (uint32_t *)(buffer + entry_offset);
    return (*entry) & 0x0FFFFFFF;
}

/* Initialize FAT32 filesystem */
bool fat32_init(void) {
    /* Read boot sector */
    if (!ata_read_sectors(0, 1, (uint8_t *)&boot_sector)) {
        return false;
    }
    
    /* Verify FAT32 signature */
    if (strcmp(boot_sector.filesystem_type, "FAT32   ") != 0) {
        /* Try to initialize anyway if filesystem type is not set */
        if (boot_sector.sectors_per_fat_32 == 0) {
            return false;
        }
    }
    
    /* Calculate important values */
    fat_begin_lba = boot_sector.reserved_sectors;
    cluster_begin_lba = fat_begin_lba + (boot_sector.fat_count * boot_sector.sectors_per_fat_32);
    sectors_per_cluster = boot_sector.sectors_per_cluster;
    root_dir_first_cluster = boot_sector.root_cluster;
    
    fs_initialized = true;
    return true;
}

/* Read directory entries from a cluster */
static bool read_directory_cluster(uint32_t cluster, fat32_dir_entry_t *entries, uint32_t *count, uint32_t max_count) {
    uint32_t lba = cluster_to_lba(cluster);
    uint8_t buffer[512];
    uint32_t entry_count = 0;
    
    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        if (!ata_read_sectors(lba + i, 1, buffer)) {
            return false;
        }
        
        fat32_dir_entry_t *dir_entries = (fat32_dir_entry_t *)buffer;
        for (int j = 0; j < 16; j++) {  /* 512 / 32 = 16 entries per sector */
            if (dir_entries[j].name[0] == 0x00) {
                /* End of directory */
                *count = entry_count;
                return true;
            }
            
            if ((uint8_t)dir_entries[j].name[0] == 0xE5) {
                /* Deleted entry, skip */
                continue;
            }
            
            if (dir_entries[j].attributes == FAT_ATTR_LONG_NAME) {
                /* Long filename entry, skip for now */
                continue;
            }
            
            if (entry_count < max_count) {
                memcpy(&entries[entry_count], &dir_entries[j], sizeof(fat32_dir_entry_t));
                entry_count++;
            }
        }
    }
    
    *count = entry_count;
    return true;
}

/* Find a file in directory */
static bool find_file_in_directory(uint32_t dir_cluster, const char *filename, fat32_dir_entry_t *entry) {
    char fat_name[11];
    convert_to_fat_name(filename, fat_name);
    
    uint32_t current_cluster = dir_cluster;
    uint8_t buffer[512];
    
    while (current_cluster < 0x0FFFFFF8) {
        uint32_t lba = cluster_to_lba(current_cluster);
        
        for (uint32_t i = 0; i < sectors_per_cluster; i++) {
            if (!ata_read_sectors(lba + i, 1, buffer)) {
                return false;
            }
            
            fat32_dir_entry_t *dir_entries = (fat32_dir_entry_t *)buffer;
            for (int j = 0; j < 16; j++) {
                if (dir_entries[j].name[0] == 0x00) {
                    return false;  /* End of directory */
                }
                
                if ((uint8_t)dir_entries[j].name[0] == 0xE5 ||
                    dir_entries[j].attributes == FAT_ATTR_LONG_NAME) {
                    continue;
                }
                
                if (memcmp(dir_entries[j].name, fat_name, 11) == 0) {
                    memcpy(entry, &dir_entries[j], sizeof(fat32_dir_entry_t));
                    return true;
                }
            }
        }
        
        /* Get next cluster */
        current_cluster = read_fat_entry(current_cluster);
    }
    
    return false;
}

/* List directory contents */
bool fat32_list_directory(const char *path, fat32_dir_entry_t *entries, uint32_t *count) {
    if (!fs_initialized) return false;
    
    uint32_t dir_cluster = root_dir_first_cluster;
    
    /* For simplicity, only support root directory for now */
    if (path[0] != '/' || path[1] != '\0') {
        return false;
    }
    
    return read_directory_cluster(dir_cluster, entries, count, 128);
}

/* Check if file exists */
bool fat32_file_exists(const char *path) {
    if (!fs_initialized) return false;
    
    /* Skip leading slash */
    if (path[0] == '/') path++;
    
    fat32_dir_entry_t entry;
    return find_file_in_directory(root_dir_first_cluster, path, &entry);
}

/* Get file size */
uint32_t fat32_get_file_size(const char *path) {
    if (!fs_initialized) return 0;
    
    if (path[0] == '/') path++;
    
    fat32_dir_entry_t entry;
    if (!find_file_in_directory(root_dir_first_cluster, path, &entry)) {
        return 0;
    }
    
    return entry.file_size;
}

/* Read file contents */
bool fat32_read_file(const char *path, uint8_t *buffer, uint32_t *size) {
    if (!fs_initialized) return false;
    
    if (path[0] == '/') path++;
    
    /* Find file entry */
    fat32_dir_entry_t entry;
    if (!find_file_in_directory(root_dir_first_cluster, path, &entry)) {
        return false;
    }
    
    /* Get first cluster */
    uint32_t cluster = ((uint32_t)entry.first_cluster_high << 16) | entry.first_cluster_low;
    uint32_t bytes_read = 0;
    uint32_t bytes_per_cluster = sectors_per_cluster * 512;
    
    /* Read file data */
    while (cluster < 0x0FFFFFF8 && bytes_read < entry.file_size) {
        uint32_t lba = cluster_to_lba(cluster);
        uint32_t to_read = (entry.file_size - bytes_read > bytes_per_cluster) ? 
                           bytes_per_cluster : (entry.file_size - bytes_read);
        
        if (!ata_read_sectors(lba, sectors_per_cluster, buffer + bytes_read)) {
            return false;
        }
        
        bytes_read += to_read;
        cluster = read_fat_entry(cluster);
    }
    
    *size = bytes_read;
    return true;
}

/* Write file (simplified - not fully implemented) */
bool fat32_write_file(const char *path, const uint8_t *buffer, uint32_t size) {
    (void)path;
    (void)buffer;
    (void)size;
    return false;  /* TODO: Implement write support */
}

/* Create file (simplified - not fully implemented) */
bool fat32_create_file(const char *path) {
    (void)path;
    return false;  /* TODO: Implement file creation */
}

/* Delete file (simplified - not fully implemented) */
bool fat32_delete_file(const char *path) {
    (void)path;
    return false;  /* TODO: Implement file deletion */
}
