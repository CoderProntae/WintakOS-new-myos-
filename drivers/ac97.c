#include "ac97.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

/* AC97 Mixer Registers (NAMBAR) */
#define AC97_RESET          0x00
#define AC97_MASTER_VOL     0x02
#define AC97_PCM_VOL        0x18
#define AC97_SAMPLE_RATE    0x2C

/* AC97 Bus Master Registers (NABMBAR) */
#define AC97_PO_BDBAR       0x10  /* PCM Out Buffer Descriptor BAR */
#define AC97_PO_CIV         0x14  /* Current Index Value */
#define AC97_PO_LVI         0x15  /* Last Valid Index */
#define AC97_PO_SR          0x16  /* Status Register */
#define AC97_PO_CR          0x1B  /* Control Register */
#define AC97_GLOB_CNT       0x2C  /* Global Control */
#define AC97_GLOB_STA       0x30  /* Global Status */

/* BDL Entry */
typedef struct {
    uint32_t addr;
    uint16_t length;   /* sample sayisi */
    uint16_t flags;    /* bit14=last, bit15=IOC */
} __attribute__((packed)) ac97_bdl_t;

#define AC97_SAMPLE_RATE_HZ  48000
#define AC97_BDL_COUNT       32
#define AC97_BUF_SAMPLES     4800  /* 100ms @ 48kHz */

static bool         ac97_found = false;
static uint16_t     nambar = 0;   /* mixer I/O base */
static uint16_t     nabmbar = 0;  /* bus master I/O base */

/* DMA icin statik bufferlar — 16-byte hizali */
static ac97_bdl_t   bdl[AC97_BDL_COUNT] __attribute__((aligned(16)));
static int16_t      sample_buf[AC97_BUF_SAMPLES * 2] __attribute__((aligned(16))); /* stereo */

static uint32_t     current_freq = 0;

static void ac97_mixer_write(uint8_t reg, uint16_t val)
{
    outw(nambar + reg, val);
}

static uint16_t ac97_mixer_read(uint8_t reg)
{
    return inw(nambar + reg);
}

static void ac97_nabm_writeb(uint8_t reg, uint8_t val)
{
    outb(nabmbar + reg, val);
}

static void ac97_nabm_writel(uint8_t reg, uint32_t val)
{
    outl(nabmbar + reg, val);
}

static void generate_tone(uint32_t frequency)
{
    if (frequency == 0) {
        memset(sample_buf, 0, sizeof(sample_buf));
        return;
    }

    uint32_t half_period = AC97_SAMPLE_RATE_HZ / (2 * frequency);
    if (half_period == 0) half_period = 1;

    int16_t amplitude = 24000;

    for (uint32_t i = 0; i < AC97_BUF_SAMPLES; i++) {
        uint32_t pos = i % (half_period * 2);
        int16_t val = (pos < half_period) ? amplitude : -amplitude;
        sample_buf[i * 2]     = val;  /* left */
        sample_buf[i * 2 + 1] = val;  /* right */
    }
}

bool ac97_init(void)
{
    pci_device_t dev;

    /* AC97 Audio: class 0x04 (multimedia), subclass 0x01 (audio) */
    if (!pci_find_device(0x04, 0x01, &dev)) {
        ac97_found = false;
        return false;
    }

    /* BAR0 = NAMBAR, BAR1 = NABMBAR (I/O space, bit 0 = 1) */
    nambar  = (uint16_t)(dev.bar0 & 0xFFFE);
    nabmbar = (uint16_t)(dev.bar1 & 0xFFFE);

    if (nambar == 0 || nabmbar == 0) {
        ac97_found = false;
        return false;
    }

    /* PCI Bus Master etkinlestir */
    pci_enable_bus_master(&dev);

    /* Codec reset */
    ac97_mixer_write(AC97_RESET, 0);

    /* Kisa bekleme */
    for (volatile int i = 0; i < 100000; i++);

    /* Global Control: Cold Reset temizle, AC97 2.0 */
    ac97_nabm_writel(AC97_GLOB_CNT, 0x02);
    for (volatile int i = 0; i < 100000; i++);

    /* Ses seviyesi: 0 = max, 0x8000 = mute */
    ac97_mixer_write(AC97_MASTER_VOL, 0x0000); /* Max master */
    ac97_mixer_write(AC97_PCM_VOL, 0x0808);    /* PCM biraz kisik */

    /* Variable Rate destegi kontrol */
    /* Extended Audio Status/Control (reg 0x28) bit 0 = VRA */
    uint16_t ext = ac97_mixer_read(0x28);
    if (ext & 0x01) {
        ac97_mixer_write(AC97_SAMPLE_RATE, AC97_SAMPLE_RATE_HZ);
    }

    /* PCM Out durdur */
    ac97_nabm_writeb(AC97_PO_CR, 0x02); /* Reset */
    for (volatile int i = 0; i < 10000; i++);
    ac97_nabm_writeb(AC97_PO_CR, 0x00);

    /* Buffer temizle */
    memset(sample_buf, 0, sizeof(sample_buf));
    memset(bdl, 0, sizeof(bdl));

    /* BDL ayarla — tum girisler ayni buffera isaret eder (dongu) */
    for (int i = 0; i < AC97_BDL_COUNT; i++) {
        bdl[i].addr   = (uint32_t)sample_buf;
        bdl[i].length = AC97_BUF_SAMPLES;  /* her kanal icin sample sayisi */
        bdl[i].flags  = 0;
    }
    /* Son giriste dongu isaretleme */
    bdl[AC97_BDL_COUNT - 1].flags = (1 << 14); /* Last entry */

    /* BDL adresini yaz */
    ac97_nabm_writel(AC97_PO_BDBAR, (uint32_t)bdl);

    /* LVI = son gecerli indeks */
    ac97_nabm_writeb(AC97_PO_LVI, AC97_BDL_COUNT - 1);

    ac97_found = true;
    current_freq = 0;

    return true;
}

void ac97_play_tone(uint32_t frequency)
{
    if (!ac97_found) return;
    if (frequency == current_freq) return;
    current_freq = frequency;

    /* Durdur */
    ac97_nabm_writeb(AC97_PO_CR, 0x02);
    for (volatile int i = 0; i < 5000; i++);
    ac97_nabm_writeb(AC97_PO_CR, 0x00);

    /* Yeni ton uret */
    generate_tone(frequency);

    /* BDL guncelle */
    for (int i = 0; i < AC97_BDL_COUNT; i++) {
        bdl[i].addr   = (uint32_t)sample_buf;
        bdl[i].length = AC97_BUF_SAMPLES;
        bdl[i].flags  = 0;
    }
    bdl[AC97_BDL_COUNT - 1].flags = (1 << 14);

    ac97_nabm_writel(AC97_PO_BDBAR, (uint32_t)bdl);
    ac97_nabm_writeb(AC97_PO_LVI, AC97_BDL_COUNT - 1);

    if (frequency > 0) {
        /* Baslat: Run + Loop */
        ac97_nabm_writeb(AC97_PO_CR, 0x01);
    }
}

void ac97_stop(void)
{
    if (!ac97_found) return;
    ac97_nabm_writeb(AC97_PO_CR, 0x00);
    current_freq = 0;
}

bool ac97_available(void)
{
    return ac97_found;
}
