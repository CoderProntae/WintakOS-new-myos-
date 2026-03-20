#include "ahci.h"
#include "../cpu/pci.h"
#include "../lib/string.h"

/* PCI class/subclass */
#define AHCI_CLASS    0x01
#define AHCI_SUBCLASS 0x06

/* HBA register offsets */
#define HBA_GHC  0x04
#define HBA_PI   0x0C

/* Port register offsets (base = ABAR + 0x100 + port*0x80) */
#define P_CLB    0x00
#define P_CLBU   0x04
#define P_FB     0x08
#define P_FBU    0x0C
#define P_IS     0x10
#define P_CMD    0x18
#define P_TFD    0x20
#define P_SIG    0x24
#define P_SSTS   0x28
#define P_SERR   0x30
#define P_CI     0x38

/* CMD bits */
#define CMD_ST   (1u << 0)
#define CMD_FRE  (1u << 4)
#define CMD_FR   (1u << 14)
#define CMD_CR   (1u << 15)

/* GHC bits */
#define GHC_AE   (1u << 31)

#define FIS_REG_H2D 0x27

static uint32_t abar = 0;
static uint32_t port_base = 0;
static ata_drive_t ahci_drv;
static bool ahci_ok = false;

/* DMA tamponlar — hizali */
static uint8_t clb_mem[1024]  __attribute__((aligned(1024)));
static uint8_t fis_mem[256]   __attribute__((aligned(256)));
static uint8_t tbl_mem[256]   __attribute__((aligned(128)));
static uint8_t dma_buf[4096]  __attribute__((aligned(512)));

/* ---- MMIO ---- */

static inline uint32_t mr(uint32_t base, uint32_t off)
{
    return *(volatile uint32_t*)(base + off);
}

static inline void mw(uint32_t base, uint32_t off, uint32_t val)
{
    *(volatile uint32_t*)(base + off) = val;
}

/* ---- Port engine ---- */

static void port_stop(uint32_t pb)
{
    uint32_t cmd = mr(pb, P_CMD);
    cmd &= ~CMD_ST;
    mw(pb, P_CMD, cmd);
    for (uint32_t t = 0; t < 500000; t++) {
        if (!(mr(pb, P_CMD) & CMD_CR)) break;
    }
    cmd = mr(pb, P_CMD);
    cmd &= ~CMD_FRE;
    mw(pb, P_CMD, cmd);
    for (uint32_t t = 0; t < 500000; t++) {
        if (!(mr(pb, P_CMD) & CMD_FR)) break;
    }
}

static void port_start(uint32_t pb)
{
    for (uint32_t t = 0; t < 500000; t++) {
        if (!(mr(pb, P_TFD) & 0x88)) break;
    }
    uint32_t cmd = mr(pb, P_CMD);
    cmd |= CMD_FRE;
    mw(pb, P_CMD, cmd);
    cmd |= CMD_ST;
    mw(pb, P_CMD, cmd);
}

/* ---- Komut gonder ---- */

static bool send_cmd(uint8_t ata_cmd, uint32_t lba, uint16_t count,
                      uint32_t byte_count, bool write)
{
    /* IS temizle */
    mw(port_base, P_IS, 0xFFFFFFFF);

    /* Command header (slot 0, 32 byte) */
    memset(clb_mem, 0, 32);
    uint32_t* hdr = (uint32_t*)clb_mem;
    uint32_t dw0 = 5;          /* CFL = 5 DWORD */
    dw0 |= (1u << 10);        /* C = clear busy */
    dw0 |= (1u << 16);        /* PRDTL = 1 */
    if (write) {
        dw0 |= (1u << 6);     /* W = write */
    }
    hdr[0] = dw0;
    hdr[1] = 0;               /* PRDBC */
    hdr[2] = (uint32_t)tbl_mem;  /* CTBA */
    hdr[3] = 0;               /* CTBAU */

    /* Command table */
    memset(tbl_mem, 0, 256);

    /* FIS Register H2D (offset 0) */
    tbl_mem[0] = FIS_REG_H2D;
    tbl_mem[1] = 0x80;        /* C=1 (command) */
    tbl_mem[2] = ata_cmd;
    tbl_mem[3] = 0;           /* features */
    tbl_mem[4] = (uint8_t)(lba & 0xFF);
    tbl_mem[5] = (uint8_t)((lba >> 8) & 0xFF);
    tbl_mem[6] = (uint8_t)((lba >> 16) & 0xFF);
    tbl_mem[7] = 0x40;        /* LBA mode */
    tbl_mem[8] = (uint8_t)((lba >> 24) & 0xFF);
    tbl_mem[9] = 0;
    tbl_mem[10] = 0;
    tbl_mem[11] = 0;
    tbl_mem[12] = (uint8_t)(count & 0xFF);
    tbl_mem[13] = (uint8_t)((count >> 8) & 0xFF);

    /* PRDT entry 0 (offset 0x80) */
    uint32_t* prdt = (uint32_t*)(tbl_mem + 0x80);
    prdt[0] = (uint32_t)dma_buf;   /* DBA */
    prdt[1] = 0;                   /* DBAU */
    prdt[2] = 0;
    prdt[3] = byte_count - 1;     /* DBC (0-based) */

    /* Komutu yayinla (slot 0) */
    mw(port_base, P_CI, 1);

    /* Tamamlanmasini bekle */
    for (uint32_t t = 0; t < 1000000; t++) {
        uint32_t ci = mr(port_base, P_CI);
        if (!(ci & 1)) break;
        uint32_t is = mr(port_base, P_IS);
        if (is & (1u << 30)) return false;
    }

    /* Hata kontrol */
    if (mr(port_base, P_CI) & 1) return false;
    if (mr(port_base, P_TFD) & 0x01) return false;

    return true;
}

/* ---- Public API ---- */

bool ahci_init(void)
{
    pci_device_t dev;
    memset(&ahci_drv, 0, sizeof(ahci_drv));
    ahci_ok = false;

    if (!pci_find_device(AHCI_CLASS, AHCI_SUBCLASS, &dev))
        return false;

    /* BAR5 oku (ABAR) */
    uint32_t bar5 = pci_config_read(dev.bus, dev.slot, dev.func, 0x24);
    abar = bar5 & ~0xFu;
    if (abar == 0) return false;

    /* Bus mastering + memory space etkinlestir */
    pci_enable_bus_master(&dev);
    uint32_t pcicmd = pci_config_read(dev.bus, dev.slot, dev.func, 0x04);
    pcicmd |= (1 << 1);  /* Memory Space Enable */
    pci_config_write(dev.bus, dev.slot, dev.func, 0x04, pcicmd);

    /* AHCI modu etkinlestir */
    uint32_t ghc = mr(abar, HBA_GHC);
    ghc |= GHC_AE;
    mw(abar, HBA_GHC, ghc);

    /* Portlari tara */
    uint32_t pi = mr(abar, HBA_PI);

    for (int p = 0; p < 32; p++) {
        if (!(pi & (1u << p))) continue;

        uint32_t pb = abar + 0x100 + (uint32_t)p * 0x80;
        uint32_t ssts = mr(pb, P_SSTS);
        uint8_t det = ssts & 0x0F;
        uint8_t ipm = (ssts >> 8) & 0x0F;

        /* det=3 = device + phy, ipm=1 = active */
        if (det != 3 || ipm != 1) continue;

        /* Port baslat */
        port_stop(pb);

        memset(clb_mem, 0, sizeof(clb_mem));
        memset(fis_mem, 0, sizeof(fis_mem));

        mw(pb, P_CLB, (uint32_t)clb_mem);
        mw(pb, P_CLBU, 0);
        mw(pb, P_FB, (uint32_t)fis_mem);
        mw(pb, P_FBU, 0);

        mw(pb, P_SERR, 0xFFFFFFFF);
        mw(pb, P_IS, 0xFFFFFFFF);

        port_start(pb);
        port_base = pb;

        /* IDENTIFY */
        memset(dma_buf, 0, 512);
        if (!send_cmd(0xEC, 0, 0, 512, false))
            continue;

        uint16_t* id = (uint16_t*)dma_buf;

        ahci_drv.present = true;
        ahci_drv.is_ata = true;
        ahci_drv.base_port = (uint16_t)pb;
        ahci_drv.drive_num = (uint8_t)p;
        ahci_drv.channel = 2;  /* AHCI */

        /* Model (word 27-46) */
        for (int i = 0; i < 20; i++) {
            ahci_drv.model[i * 2]     = (char)(id[27 + i] >> 8);
            ahci_drv.model[i * 2 + 1] = (char)(id[27 + i] & 0xFF);
        }
        ahci_drv.model[40] = '\0';
        for (int i = 39; i >= 0; i--) {
            if (ahci_drv.model[i] == ' ' || ahci_drv.model[i] == '\0')
                ahci_drv.model[i] = '\0';
            else
                break;
        }

        /* Serial (word 10-19) */
        for (int i = 0; i < 10; i++) {
            ahci_drv.serial[i * 2]     = (char)(id[10 + i] >> 8);
            ahci_drv.serial[i * 2 + 1] = (char)(id[10 + i] & 0xFF);
        }
        ahci_drv.serial[20] = '\0';
        for (int i = 19; i >= 0; i--) {
            if (ahci_drv.serial[i] == ' ' || ahci_drv.serial[i] == '\0')
                ahci_drv.serial[i] = '\0';
            else
                break;
        }

        /* Sectors */
        ahci_drv.sectors = (uint32_t)id[60] | ((uint32_t)id[61] << 16);
        if (ahci_drv.sectors == 0)
            ahci_drv.sectors = (uint32_t)id[100] | ((uint32_t)id[101] << 16);
        ahci_drv.size_mb = ahci_drv.sectors / 2048;

        if (ahci_drv.sectors > 16) {
            ahci_ok = true;
            return true;
        }
    }

    return false;
}

bool ahci_is_present(void) { return ahci_ok; }

ata_drive_t* ahci_get_info(void)
{
    if (!ahci_ok) return NULL;
    return &ahci_drv;
}

bool ahci_read_sectors(uint32_t lba, uint8_t count, void* buf)
{
    if (!ahci_ok || count == 0) return false;

    uint8_t rem = count;
    uint32_t cur = lba;
    uint8_t* dst = (uint8_t*)buf;

    while (rem > 0) {
        uint8_t batch = (rem > 8) ? 8 : rem;
        uint32_t bytes = (uint32_t)batch * 512;

        memset(dma_buf, 0, bytes);
        if (!send_cmd(0x25, cur, batch, bytes, false))
            return false;

        memcpy(dst, dma_buf, bytes);
        rem -= batch;
        cur += batch;
        dst += bytes;
    }
    return true;
}

bool ahci_write_sectors(uint32_t lba, uint8_t count, const void* buf)
{
    if (!ahci_ok || count == 0) return false;

    uint8_t rem = count;
    uint32_t cur = lba;
    const uint8_t* src = (const uint8_t*)buf;

    while (rem > 0) {
        uint8_t batch = (rem > 8) ? 8 : rem;
        uint32_t bytes = (uint32_t)batch * 512;

        memcpy(dma_buf, src, bytes);
        if (!send_cmd(0x35, cur, batch, bytes, true))
            return false;

        rem -= batch;
        cur += batch;
        src += bytes;
    }
    return true;
}
