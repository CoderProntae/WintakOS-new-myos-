#include "ata.h"
#include "../lib/string.h"

/* ---- Port I/O ---- */

static inline uint8_t ata_inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void ata_outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t ata_inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void ata_outw(uint16_t port, uint16_t val)
{
    __asm__ volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

/* ---- ATA Register Offsets ---- */

#define ATA_REG_DATA       0
#define ATA_REG_ERROR      1
#define ATA_REG_FEATURES   1
#define ATA_REG_SECCOUNT   2
#define ATA_REG_LBA_LO     3
#define ATA_REG_LBA_MID    4
#define ATA_REG_LBA_HI     5
#define ATA_REG_DRIVE      6
#define ATA_REG_STATUS     7
#define ATA_REG_COMMAND    7

/* Control ports */
#define ATA_PRIMARY_CTRL   0x3F6
#define ATA_SECONDARY_CTRL 0x376

/* Status bits */
#define ATA_SR_BSY   0x80
#define ATA_SR_DRDY  0x40
#define ATA_SR_DF    0x20
#define ATA_SR_DRQ   0x08
#define ATA_SR_ERR   0x01

/* Commands */
#define ATA_CMD_IDENTIFY   0xEC
#define ATA_CMD_READ       0x20
#define ATA_CMD_WRITE      0x30
#define ATA_CMD_FLUSH      0xE7

/* Controller base ports */
static const uint16_t channel_base[2] = { 0x1F0, 0x170 };
static const uint16_t channel_ctrl[2] = { 0x3F6, 0x376 };

static ata_drive_t drives[ATA_MAX_DRIVES];
static uint32_t    drive_count = 0;

/* ---- Helpers ---- */

static void ata_delay(uint16_t base)
{
    /* 400ns bekle — status portunu 4 kez oku */
    ata_inb(base + ATA_REG_STATUS);
    ata_inb(base + ATA_REG_STATUS);
    ata_inb(base + ATA_REG_STATUS);
    ata_inb(base + ATA_REG_STATUS);
}

static bool ata_wait_ready(uint16_t base)
{
    uint32_t timeout = 500000;
    while (timeout > 0) {
        uint8_t s = ata_inb(base + ATA_REG_STATUS);
        if (!(s & ATA_SR_BSY)) return true;
        timeout--;
    }
    return false;
}

static bool ata_wait_drq(uint16_t base)
{
    uint32_t timeout = 500000;
    while (timeout > 0) {
        uint8_t s = ata_inb(base + ATA_REG_STATUS);
        if (s & ATA_SR_ERR) return false;
        if (s & ATA_SR_DF)  return false;
        if (s & ATA_SR_DRQ) return true;
        timeout--;
    }
    return false;
}

/* ---- IDENTIFY ---- */

static void ata_identify(uint8_t idx)
{
    ata_drive_t* d = &drives[idx];
    uint16_t base = d->base_port;
    uint8_t sel = (d->drive_num == 0) ? 0xA0 : 0xB0;

    /* Drive sec */
    ata_outb(base + ATA_REG_DRIVE, sel);
    ata_delay(base);

    /* Registerleri sifirla */
    ata_outb(base + ATA_REG_SECCOUNT, 0);
    ata_outb(base + ATA_REG_LBA_LO, 0);
    ata_outb(base + ATA_REG_LBA_MID, 0);
    ata_outb(base + ATA_REG_LBA_HI, 0);

    /* IDENTIFY gonder */
    ata_outb(base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_delay(base);

    uint8_t status = ata_inb(base + ATA_REG_STATUS);
    if (status == 0) {
        d->present = false;
        return;
    }

    /* BSY bekle */
    if (!ata_wait_ready(base)) {
        d->present = false;
        return;
    }

    /* ATAPI kontrol — LBA mid/hi sifir degilse ATA degil */
    uint8_t lba_mid = ata_inb(base + ATA_REG_LBA_MID);
    uint8_t lba_hi  = ata_inb(base + ATA_REG_LBA_HI);
    if (lba_mid != 0 || lba_hi != 0) {
        d->present = true;
        d->is_ata = false;
        d->sectors = 0;
        d->size_mb = 0;
        memset(d->model, 0, sizeof(d->model));
        memset(d->serial, 0, sizeof(d->serial));
        return;
    }

    /* DRQ bekle */
    if (!ata_wait_drq(base)) {
        d->present = false;
        return;
    }

    /* IDENTIFY verisini oku (256 word = 512 byte) */
    uint16_t ident[256];
    for (int i = 0; i < 256; i++)
        ident[i] = ata_inw(base + ATA_REG_DATA);

    d->present = true;
    d->is_ata = true;

    /* Model string (word 27-46, byte-swapped) */
    for (int i = 0; i < 20; i++) {
        d->model[i * 2]     = (char)(ident[27 + i] >> 8);
        d->model[i * 2 + 1] = (char)(ident[27 + i] & 0xFF);
    }
    d->model[40] = '\0';
    /* Bosluk temizle */
    for (int i = 39; i >= 0; i--) {
        if (d->model[i] == ' ' || d->model[i] == '\0')
            d->model[i] = '\0';
        else
            break;
    }

    /* Serial string (word 10-19) */
    for (int i = 0; i < 10; i++) {
        d->serial[i * 2]     = (char)(ident[10 + i] >> 8);
        d->serial[i * 2 + 1] = (char)(ident[10 + i] & 0xFF);
    }
    d->serial[20] = '\0';
    for (int i = 19; i >= 0; i--) {
        if (d->serial[i] == ' ' || d->serial[i] == '\0')
            d->serial[i] = '\0';
        else
            break;
    }

    /* Toplam sektor — LBA28 (word 60-61) */
    d->sectors = (uint32_t)ident[60] | ((uint32_t)ident[61] << 16);
    d->size_mb = d->sectors / 2048; /* 512 * 2048 = 1MB */
}

/* ---- Public API ---- */

void ata_init(void)
{
    memset(drives, 0, sizeof(drives));
    drive_count = 0;

    for (uint8_t ch = 0; ch < 2; ch++) {
        for (uint8_t dr = 0; dr < 2; dr++) {
            uint8_t idx = ch * 2 + dr;
            drives[idx].base_port = channel_base[ch];
            drives[idx].drive_num = dr;
            drives[idx].channel = ch;
            drives[idx].present = false;
            drives[idx].is_ata = false;

            /* Oncelik: floating bus kontrolu */
            uint8_t s = ata_inb(channel_base[ch] + ATA_REG_STATUS);
            if (s == 0xFF) continue; /* controller yok */

            /* Software reset */
            ata_outb(channel_ctrl[ch], 0x04);
            ata_delay(channel_base[ch]);
            ata_outb(channel_ctrl[ch], 0x00);
            ata_delay(channel_base[ch]);

            ata_identify(idx);
            if (drives[idx].present)
                drive_count++;
        }
    }
}

bool ata_is_present(uint8_t drive)
{
    if (drive >= ATA_MAX_DRIVES) return false;
    return drives[drive].present && drives[drive].is_ata;
}

ata_drive_t* ata_get_info(uint8_t drive)
{
    if (drive >= ATA_MAX_DRIVES) return NULL;
    if (!drives[drive].present) return NULL;
    return &drives[drive];
}

uint32_t ata_get_drive_count(void)
{
    return drive_count;
}

bool ata_read_sectors(uint8_t drive, uint32_t lba,
                       uint8_t count, void* buf)
{
    if (drive >= ATA_MAX_DRIVES) return false;
    ata_drive_t* d = &drives[drive];
    if (!d->present || !d->is_ata) return false;
    if (count == 0) return false;

    uint16_t base = d->base_port;
    uint8_t head = (d->drive_num == 0) ? 0xE0 : 0xF0;

    if (!ata_wait_ready(base)) return false;

    /* Drive + LBA mode + LBA bits 24-27 */
    ata_outb(base + ATA_REG_DRIVE, head | ((lba >> 24) & 0x0F));
    ata_delay(base);

    ata_outb(base + ATA_REG_SECCOUNT, count);
    ata_outb(base + ATA_REG_LBA_LO, (uint8_t)(lba & 0xFF));
    ata_outb(base + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    ata_outb(base + ATA_REG_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));

    ata_outb(base + ATA_REG_COMMAND, ATA_CMD_READ);

    uint16_t* wbuf = (uint16_t*)buf;
    for (uint8_t s = 0; s < count; s++) {
        if (!ata_wait_drq(base)) return false;
        for (int i = 0; i < 256; i++)
            wbuf[s * 256 + i] = ata_inw(base + ATA_REG_DATA);
    }

    return true;
}

bool ata_write_sectors(uint8_t drive, uint32_t lba,
                        uint8_t count, const void* buf)
{
    if (drive >= ATA_MAX_DRIVES) return false;
    ata_drive_t* d = &drives[drive];
    if (!d->present || !d->is_ata) return false;
    if (count == 0) return false;

    uint16_t base = d->base_port;
    uint8_t head = (d->drive_num == 0) ? 0xE0 : 0xF0;

    if (!ata_wait_ready(base)) return false;

    ata_outb(base + ATA_REG_DRIVE, head | ((lba >> 24) & 0x0F));
    ata_delay(base);

    ata_outb(base + ATA_REG_SECCOUNT, count);
    ata_outb(base + ATA_REG_LBA_LO, (uint8_t)(lba & 0xFF));
    ata_outb(base + ATA_REG_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
    ata_outb(base + ATA_REG_LBA_HI, (uint8_t)((lba >> 16) & 0xFF));

    ata_outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE);

    const uint16_t* wbuf = (const uint16_t*)buf;
    for (uint8_t s = 0; s < count; s++) {
        if (!ata_wait_drq(base)) return false;
        for (int i = 0; i < 256; i++)
            ata_outw(base + ATA_REG_DATA, wbuf[s * 256 + i]);

        /* Flush */
        ata_outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH);
        if (!ata_wait_ready(base)) return false;
    }

    return true;
}

/* ---- Disk Config ---- */

bool disk_config_load(disk_config_t* cfg)
{
    /* Ilk ATA diski bul */
    for (uint8_t i = 0; i < ATA_MAX_DRIVES; i++) {
        if (!ata_is_present(i)) continue;

        uint8_t sector[512];
        if (!ata_read_sectors(i, CONFIG_LBA, 1, sector))
            continue;

        disk_config_t* dc = (disk_config_t*)sector;
        if (dc->magic == CONFIG_MAGIC && dc->version == CONFIG_VERSION) {
            memcpy(cfg, sector, sizeof(disk_config_t));
            return true;
        }
    }
    return false;
}

bool disk_config_save(const disk_config_t* cfg)
{
    for (uint8_t i = 0; i < ATA_MAX_DRIVES; i++) {
        if (!ata_is_present(i)) continue;

        uint8_t sector[512];
        memset(sector, 0, 512);
        memcpy(sector, cfg, sizeof(disk_config_t));

        return ata_write_sectors(i, CONFIG_LBA, 1, sector);
    }
    return false;
}
