#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stdbool.h>

/* File types */
#define VFS_FILE      1
#define VFS_DIRECTORY 2

/* File descriptor */
typedef struct {
    char path[256];
    uint32_t position;
    uint32_t size;
    uint8_t type;
    bool open;
} vfs_file_t;

/* Directory entry */
typedef struct {
    char name[256];
    uint32_t size;
    uint8_t type;
    bool is_directory;
} vfs_dirent_t;

/* VFS operations */
void vfs_init(void);
int vfs_open(const char *path);
void vfs_close(int fd);
int vfs_read(int fd, void *buffer, uint32_t size);
int vfs_write(int fd, const void *buffer, uint32_t size);
bool vfs_exists(const char *path);
uint32_t vfs_file_size(const char *path);
int vfs_list_directory(const char *path, vfs_dirent_t *entries, uint32_t max_count);

#endif /* VFS_H */
