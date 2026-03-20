#include "vfs.h"
#include "ramfs.h"
#include "../lib/string.h"

void vfs_init(void)
{
    /* RAMFS zaten ayri baslatiliyor.
       Ileride FAT32 mount buraya eklenecek. */
}

bool vfs_exists(const char* path)
{
    return ramfs_exists(path);
}

int vfs_read(const char* path, void* buf, uint32_t size)
{
    return ramfs_read(path, buf, size);
}

int vfs_write(const char* path, const void* data, uint32_t size)
{
    return ramfs_write(path, data, size);
}

bool vfs_create(const char* path, bool is_dir)
{
    return ramfs_create(path, is_dir) >= 0;
}

bool vfs_delete(const char* path)
{
    return ramfs_delete(path);
}

uint32_t vfs_file_count(void)
{
    return ramfs_file_count();
}
