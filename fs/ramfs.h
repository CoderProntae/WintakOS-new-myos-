#ifndef WINTAKOS_RAMFS_H
#define WINTAKOS_RAMFS_H

#include "../include/types.h"

#define RAMFS_MAX_FILES     64
#define RAMFS_MAX_NAME      32
#define RAMFS_MAX_FILESIZE  8192

typedef struct {
    char     name[RAMFS_MAX_NAME];
    uint8_t  data[RAMFS_MAX_FILESIZE];
    uint32_t size;
    bool     used;
    bool     is_dir;
} ramfs_file_t;

void         ramfs_init(void);
int          ramfs_create(const char* name, bool is_dir);
int          ramfs_write(const char* name, const void* data, uint32_t size);
int          ramfs_append(const char* name, const void* data, uint32_t size);
int          ramfs_read(const char* name, void* buf, uint32_t max_size);
uint32_t     ramfs_size(const char* name);
bool         ramfs_exists(const char* name);
bool         ramfs_delete(const char* name);
ramfs_file_t* ramfs_get_file(uint32_t index);
uint32_t     ramfs_file_count(void);

#endif
