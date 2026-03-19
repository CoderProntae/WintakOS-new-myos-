/*============================================================================
 * WintakOS - framebuffer.c
 * VESA Framebuffer Surucusu
 *
 * Multiboot2 bilgi yapisindan framebuffer adresini ve
 * ozelliklerini okur, piksel cizim fonksiyonlari saglar.
 *==========================================================================*/

#include "framebuffer.h"
#include "../lib/string.h"

static framebuffer_t fb;

/* Multiboot2 tag tipleri */
#define MB2_TAG_END         0
#define MB2_TAG_FRAMEBUFFER 8

/* Multiboot2 tag basligi */
typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_t;

/* Framebuffer tag */
typedef struct {
    uint32_t type;
    uint32_t size;
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t  bpp;
    uint8_t  fb_type;
} __attribute__((packed)) mb2_tag_fb_t;

bool fb_init(void* mbi_ptr)
{
    fb.available = false;

    if (!mbi_ptr) return false;

    /* Multiboot2 info yapisi:
     *   ilk 4 byte: toplam boyut
     *   sonraki 4 byte: reserved
     *   ardindan tag listesi */
    uint32_t mbi_size = *(uint32_t*)mbi_ptr;
    (void)mbi_size;

    uint8_t* ptr = (uint8_t*)mbi_ptr + 8;

    while (1) {
        mb2_tag_t* tag = (mb2_tag_t*)ptr;

        if (tag->type == MB2_TAG_END) break;

        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            mb2_tag_fb_t* fb_tag = (mb2_tag_fb_t*)tag;

            fb.address   = (uint32_t*)(uint32_t)fb_tag->addr;
            fb.width     = fb_tag->width;
            fb.height    = fb_tag->height;
            fb.pitch     = fb_tag->pitch;
            fb.bpp       = fb_tag->bpp;
            fb.available = true;

            return true;
        }

        /* Sonraki tag: boyutu 8-byte hizala */
        uint32_t tag_size = (tag->size + 7) & ~7;
        ptr += tag_size;
    }

    return false;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!fb.available) return;
    if (x >= fb.width || y >= fb.height) return;

    uint32_t offset = y * (fb.pitch / 4) + x;
    fb.address[offset] = color;
}

uint32_t fb_get_pixel(uint32_t x, uint32_t y)
{
    if (!fb.available) return 0;
    if (x >= fb.width || y >= fb.height) return 0;

    uint32_t offset = y * (fb.pitch / 4) + x;
    return fb.address[offset];
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!fb.available) return;

    for (uint32_t row = y; row < y + h && row < fb.height; row++) {
        uint32_t offset = row * (fb.pitch / 4) + x;
        for (uint32_t col = 0; col < w && (x + col) < fb.width; col++) {
            fb.address[offset + col] = color;
        }
    }
}

void fb_clear(uint32_t color)
{
    if (!fb.available) return;

    uint32_t total = fb.height * (fb.pitch / 4);
    for (uint32_t i = 0; i < total; i++) {
        fb.address[i] = color;
    }
}

void fb_scroll(uint32_t lines, uint32_t bg_color)
{
    if (!fb.available) return;

    uint32_t pixel_lines = lines;
    uint32_t pitch32 = fb.pitch / 4;

    /* Yukari kaydir */
    uint32_t copy_rows = fb.height - pixel_lines;
    for (uint32_t row = 0; row < copy_rows; row++) {
        uint32_t dst = row * pitch32;
        uint32_t src = (row + pixel_lines) * pitch32;
        memcpy(&fb.address[dst], &fb.address[src], fb.pitch);
    }

    /* Alt kismi temizle */
    for (uint32_t row = copy_rows; row < fb.height; row++) {
        uint32_t offset = row * pitch32;
        for (uint32_t col = 0; col < fb.width; col++) {
            fb.address[offset + col] = bg_color;
        }
    }
}

framebuffer_t* fb_get_info(void)
{
    return &fb;
}
