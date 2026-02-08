#include "vfs.h"
#include "memory.h"
#include "../../drivers/include/fat32.h"
#include <stdint.h>
#include <stdbool.h>

/* Maximum open files */
#define MAX_OPEN_FILES 32

/* File descriptor table */
static vfs_file_t file_table[MAX_OPEN_FILES];

/* String helpers */
static void strcpy(char *dest, const char *src) {
    int i = 0;
    while (src[i]) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

/* Initialize VFS */
void vfs_init(void) {
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        file_table[i].open = false;
    }
    
    /* Initialize FAT32 filesystem */
    fat32_init();
}

/* Open a file */
int vfs_open(const char *path) {
    /* Find free file descriptor */
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (!file_table[i].open) {
            fd = i;
            break;
        }
    }
    
    if (fd == -1) {
        return -1;  /* No free descriptors */
    }
    
    /* Check if file exists */
    if (!fat32_file_exists(path)) {
        return -1;
    }
    
    /* Initialize file descriptor */
    strcpy(file_table[fd].path, path);
    file_table[fd].position = 0;
    file_table[fd].size = fat32_get_file_size(path);
    file_table[fd].type = VFS_FILE;
    file_table[fd].open = true;
    
    return fd;
}

/* Close a file */
void vfs_close(int fd) {
    if (fd >= 0 && fd < MAX_OPEN_FILES) {
        file_table[fd].open = false;
    }
}

/* Read from file */
int vfs_read(int fd, void *buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].open) {
        return -1;
    }
    
    vfs_file_t *file = &file_table[fd];
    
    /* Allocate temporary buffer for full file */
    uint8_t *temp = (uint8_t *)kmalloc(file->size);
    if (!temp) {
        return -1;
    }
    
    /* Read entire file */
    uint32_t bytes_read = 0;
    if (!fat32_read_file(file->path, temp, &bytes_read)) {
        kfree(temp);
        return -1;
    }
    
    /* Copy requested portion */
    uint32_t available = file->size - file->position;
    uint32_t to_read = (size < available) ? size : available;
    
    memcpy(buffer, temp + file->position, to_read);
    file->position += to_read;
    
    kfree(temp);
    return (int)to_read;
}

/* Write to file */
int vfs_write(int fd, const void *buffer, uint32_t size) {
    if (fd < 0 || fd >= MAX_OPEN_FILES || !file_table[fd].open) {
        return -1;
    }
    
    /* Write support not fully implemented yet */
    (void)buffer;
    (void)size;
    return -1;
}

/* Check if file exists */
bool vfs_exists(const char *path) {
    return fat32_file_exists(path);
}

/* Get file size */
uint32_t vfs_file_size(const char *path) {
    return fat32_get_file_size(path);
}

/* List directory contents */
int vfs_list_directory(const char *path, vfs_dirent_t *entries, uint32_t max_count) {
    fat32_dir_entry_t fat_entries[128];
    uint32_t count = 0;
    
    if (!fat32_list_directory(path, fat_entries, &count)) {
        return -1;
    }
    
    uint32_t copied = (count < max_count) ? count : max_count;
    
    for (uint32_t i = 0; i < copied; i++) {
        /* Copy name (8.3 format to normal) */
        int j = 0;
        for (int k = 0; k < 8 && fat_entries[i].name[k] != ' '; k++) {
            entries[i].name[j++] = fat_entries[i].name[k];
        }
        
        if (fat_entries[i].name[8] != ' ') {
            entries[i].name[j++] = '.';
            for (int k = 8; k < 11 && fat_entries[i].name[k] != ' '; k++) {
                entries[i].name[j++] = fat_entries[i].name[k];
            }
        }
        entries[i].name[j] = '\0';
        
        entries[i].size = fat_entries[i].file_size;
        entries[i].is_directory = (fat_entries[i].attributes & FAT_ATTR_DIRECTORY) != 0;
        entries[i].type = entries[i].is_directory ? VFS_DIRECTORY : VFS_FILE;
    }
    
    return (int)copied;
}
