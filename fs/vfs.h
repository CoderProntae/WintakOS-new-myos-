#ifndef WINTAKOS_VFS_H
#define WINTAKOS_VFS_H

#include "../include/types.h"

/* Dosya sistemi tipi */
#define VFS_TYPE_RAMFS  0
#define VFS_TYPE_FAT32  1   /* ileride */

/* VFS */
void     vfs_init(void);
bool     vfs_exists(const char* path);
int      vfs_read(const char* path, void* buf, uint32_t size);
int      vfs_write(const char* path, const void* data, uint32_t size);
bool     vfs_create(const char* path, bool is_dir);
bool     vfs_delete(const char* path);
uint32_t vfs_file_count(void);

#endif
