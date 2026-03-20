#include "ac97.h"
#include "../cpu/pci.h"
#include "../cpu/ports.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

#define AC97_RESET          0x00
#define AC97_MASTER_VOL     0x02
#define AC97_PCM_VOL        0x18
#define AC97_SAMPLE_RATE    0x2C

#define AC97_PO_BDBAR       0x10
#define AC97_PO_LVI         0x15
#define AC97_PO_CR          0x1B
#define AC97_GLOB_CNT       0x2C

typedef struct {
    uint32_t addr;
    uint16_t length;
    uint16_t flags;
} __attribute__((packed)) ac97_bdl_t;

#define AC97_SAMPLE_RATE_HZ  48000
#define AC97_BDL_COUNT       32
#define AC97_BUF_SAMPLES     4800

static bool         ac97_found = false;
static uint16_t     nambar = 0;
static uint16_t     nabmbar = 0;

static ac97_bdl_t   bdl[AC97_BDL_COUNT] __attribute__((aligned(16)));
static int16_t      sample_buf[AC97_BUF_SAMPLES * 2] __attribute__((aligned(16)));

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

/*
 * Ucgen dalga uretimi — kare dalgadan cok daha yumusak ses verir.
 * Amplitud dusuk tutularak kulak rahatsizligi onlenir.
 */
static void generate_tone(uint32_t frequency)
{
    if (frequency == 0) {
        memset(sample_buf, 0, sizeof(sample_buf));
        return;
    }

    uint32_t period = AC97_SAMPLE_RATE_HZ / frequency;
    if (period == 0) period = 1;
    uint32_t half = period / 2;
    if (half == 0) half = 1;

    /* Dusuk amplitud: 8000 (max 32767'nin ~%25'i) */
    int16_t amplitude = 8000;

    for (uint32_t i = 0; i < AC97_BUF_SAMPLES; i++) {
        uint32_t pos = i % period;
        int16_t val;

        /* Ucgen dalga: yukari-asagi dogrusal gecis */
        if (pos < half) {
            /* 0'dan amplitude'a cik */
            val = (int16_t)((int32_t)amplitude * 2 * (int32_t)pos / (int32_t)half - amplitude);
        } else {
            /* amplitude'dan 0'a in */
            val = (int16_t)(amplitude - (int32_t)amplitude * 2 * ((int32_t)pos - (int32_t)half) / (int32_t)half);
        }

        sample_buf[i * 2]     = val;
        sample_buf[i * 2 + 1] = val;
    }

    /* Fade-in/fade-out: ilk ve son 200 sample */
    uint32_t fade = 200;
    if (fade > AC97_BUF_SAMPLES / 2) fade = AC97_BUF_SAMPLES / 2;

    for (uint32_t i = 0; i < fade; i++) {
        int32_t factor = (int32_t)i * 256 / (int32_t)fade;
        sample_buf[i * 2]     = (int16_t)(sample_buf[i * 2] * factor / 256);
        sample_buf[i * 2 + 1] = (int16_t)(sample_buf[i * 2 + 1] * factor / 256);

        uint32_t j = AC97_BUF_SAMPLES - 1 - i;
        sample_buf[j * 2]     = (int16_t)(sample_buf[j * 2] * factor / 256);
        sample_buf[j * 2 + 1] = (int16_t)(sample_buf[j * 2 + 1] * factor / 256);
    }
}

bool ac97_init(void)
{
    pci_device_t dev;

    if (!pci_find_device(0x04, 0x01, &dev)) {
        ac97_found = false;
        return false;
    }

    nambar  = (uint16_t)(dev.bar0 & 0xFFFE);
    nabmbar = (uint16_t)(dev.bar1 & 0xFFFE);

    if (nambar == 0 || nabmbar == 0) {
        ac97_found = false;
        return false;
    }

    pci_enable_bus_master(&dev);

    ac97_mixer_write(AC97_RESET, 0);
    for (volatile int i = 0; i < 100000; i++);

    ac97_nabm_writel(AC97_GLOB_CNT, 0x02);
    for (volatile int i = 0; i < 100000; i++);

    /* Ses seviyesi — biraz kisik (0x1010 = ~%75) */
    ac97_mixer_write(AC97_MASTER_VOL, 0x0404);
    ac97_mixer_write(AC97_PCM_VOL, 0x0808);

    uint16_t ext = ac97_mixer_read(0x28);
    if (ext & 0x01) {
        ac97_mixer_write(AC97_SAMPLE_RATE, AC97_SAMPLE_RATE_HZ);
    }

    ac97_nabm_writeb(AC97_PO_CR, 0x02);
    for (volatile int i = 0; i < 10000; i++);
    ac97_nabm_writeb(AC97_PO_CR, 0x00);

    memset(sample_buf, 0, sizeof(sample_buf));
    memset(bdl, 0, sizeof(bdl));

    for (int i = 0; i < AC97_BDL_COUNT; i++) {
        bdl[i].addr   = (uint32_t)sample_buf;
        bdl[i].length = AC97_BUF_SAMPLES;
        bdl[i].flags  = 0;
    }
    bdl[AC97_BDL_COUNT - 1].flags = (1 << 14);

    ac97_nabm_writel(AC97_PO_BDBAR, (uint32_t)bdl);
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

    ac97_nabm_writeb(AC97_PO_CR, 0x02);
    for (volatile int i = 0; i < 5000; i++);
    ac97_nabm_writeb(AC97_PO_CR, 0x00);

    generate_tone(frequency);

    for (int i = 0; i < AC97_BDL_COUNT; i++) {
        bdl[i].addr   = (uint32_t)sample_buf;
        bdl[i].length = AC97_BUF_SAMPLES;
        bdl[i].flags  = 0;
    }
    bdl[AC97_BDL_COUNT - 1].flags = (1 << 14);

    ac97_nabm_writel(AC97_PO_BDBAR, (uint32_t)bdl);
    ac97_nabm_writeb(AC97_PO_LVI, AC97_BDL_COUNT - 1);

    if (frequency > 0) {
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
