#include "ramfs.h"
#include "../lib/string.h"

static ramfs_file_t files[RAMFS_MAX_FILES];

static bool name_eq(const char* a, const char* b)
{
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
    return *a == *b;
}

static int find_file(const char* name)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (files[i].used && name_eq(files[i].name, name))
            return i;
    }
    return -1;
}

static int find_free(void)
{
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!files[i].used) return i;
    }
    return -1;
}

void ramfs_init(void)
{
    memset(files, 0, sizeof(files));

    /* Varsayilan dosyalar */
    ramfs_create("readme.txt", false);
    ramfs_write("readme.txt", "WintakOS v0.7.0\nHobi isletim sistemi.\n'help' yazarak komutlari gorun.\n", 67);

    ramfs_create("merhaba.txt", false);
    ramfs_write("merhaba.txt", "Merhaba Dunya!\nBu bir test dosyasidir.\n", 39);

    ramfs_create("sistem.log", false);
    ramfs_write("sistem.log", "[OK] GDT kuruldu\n[OK] IDT kuruldu\n[OK] PIC aktif\n[OK] PIT aktif\n[OK] Klavye aktif\n[OK] Mouse aktif\n[OK] RAMFS aktif\n", 113);
}

int ramfs_create(const char* name, bool is_dir)
{
    if (find_file(name) >= 0) return -1;
    int idx = find_free();
    if (idx < 0) return -2;

    uint32_t i;
    for (i = 0; i < RAMFS_MAX_NAME - 1 && name[i]; i++)
        files[idx].name[i] = name[i];
    files[idx].name[i] = '\0';
    files[idx].size = 0;
    files[idx].used = true;
    files[idx].is_dir = is_dir;
    memset(files[idx].data, 0, RAMFS_MAX_FILESIZE);

    return idx;
}

int ramfs_write(const char* name, const void* data, uint32_t size)
{
    int idx = find_file(name);
    if (idx < 0) return -1;
    if (size > RAMFS_MAX_FILESIZE) size = RAMFS_MAX_FILESIZE;

    memcpy(files[idx].data, data, size);
    files[idx].size = size;
    return (int)size;
}

int ramfs_append(const char* name, const void* data, uint32_t size)
{
    int idx = find_file(name);
    if (idx < 0) return -1;

    uint32_t space = RAMFS_MAX_FILESIZE - files[idx].size;
    if (size > space) size = space;

    memcpy(files[idx].data + files[idx].size, data, size);
    files[idx].size += size;
    return (int)size;
}

int ramfs_read(const char* name, void* buf, uint32_t max_size)
{
    int idx = find_file(name);
    if (idx < 0) return -1;

    uint32_t to_read = files[idx].size;
    if (to_read > max_size) to_read = max_size;
    memcpy(buf, files[idx].data, to_read);
    return (int)to_read;
}

uint32_t ramfs_size(const char* name)
{
    int idx = find_file(name);
    if (idx < 0) return 0;
    return files[idx].size;
}

bool ramfs_exists(const char* name)
{
    return find_file(name) >= 0;
}

bool ramfs_delete(const char* name)
{
    int idx = find_file(name);
    if (idx < 0) return false;
    files[idx].used = false;
    return true;
}

ramfs_file_t* ramfs_get_file(uint32_t index)
{
    if (index >= RAMFS_MAX_FILES) return NULL;
    if (!files[index].used) return NULL;
    return &files[index];
}

uint32_t ramfs_file_count(void)
{
    uint32_t count = 0;
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (files[i].used) count++;
    }
    return count;
}
