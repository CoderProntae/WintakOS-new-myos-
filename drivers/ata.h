#ifndef WINTAKOS_ATA_H
#define WINTAKOS_ATA_H

#include "../include/types.h"

#define ATA_MAX_DRIVES   4
#define ATA_SECTOR_SIZE  512

/* Disk config — sektor 2048'e yazilir */
#define CONFIG_MAGIC     0x574B4F53
#define CONFIG_LBA       2048
#define CONFIG_VERSION   1

typedef struct {
    bool     present;
    bool     is_ata;       /* true=ATA, false=ATAPI/diger */
    uint32_t sectors;      /* toplam sektor (LBA28) */
    uint32_t size_mb;      /* MB cinsinden boyut */
    char     model[41];
    char     serial[21];
    uint16_t base_port;
    uint8_t  drive_num;    /* 0=master, 1=slave */
    uint8_t  channel;      /* 0=primary, 1=secondary */
} ata_drive_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t pref_res_w;
    uint32_t pref_res_h;
    uint8_t  theme;
    char     username[32];
    char     reserved[512 - 49];
} __attribute__((packed)) disk_config_t;

void          ata_init(void);
bool          ata_is_present(uint8_t drive);
ata_drive_t*  ata_get_info(uint8_t drive);
uint32_t      ata_get_drive_count(void);
bool          ata_read_sectors(uint8_t drive, uint32_t lba,
                               uint8_t count, void* buf);
bool          ata_write_sectors(uint8_t drive, uint32_t lba,
                                uint8_t count, const void* buf);

/* Disk config */
bool disk_config_load(disk_config_t* cfg);
bool disk_config_save(const disk_config_t* cfg);

/* Debug icin port erisimi */
/* Port erisim — header'da tanimli, her yerden erisilebilir */
static inline uint8_t ata_port_inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void ata_port_outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t ata_port_inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

#endif
