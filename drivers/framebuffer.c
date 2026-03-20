#include "framebuffer.h"
#include "../lib/string.h"

static framebuffer_t fb;

#define FB_MAX_PIXELS (1280 * 1024)
static uint32_t backbuf[FB_MAX_PIXELS];

#define MB2_TAG_END         0
#define MB2_TAG_FRAMEBUFFER 8

typedef struct {
    uint32_t type;
    uint32_t size;
} __attribute__((packed)) mb2_tag_t;

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
    fb.backbuffer = NULL;
    fb.hw_width = 0;
    fb.hw_height = 0;
    if (!mbi_ptr) return false;

    uint8_t* ptr = (uint8_t*)mbi_ptr + 8;
    while (1) {
        mb2_tag_t* tag = (mb2_tag_t*)ptr;
        if (tag->type == MB2_TAG_END) break;
        if (tag->type == MB2_TAG_FRAMEBUFFER) {
            mb2_tag_fb_t* fb_tag = (mb2_tag_fb_t*)tag;
            fb.address   = (uint32_t*)(uint32_t)fb_tag->addr;
            fb.width     = fb_tag->width;
            fb.height    = fb_tag->height;
            fb.hw_width  = fb_tag->width;
            fb.hw_height = fb_tag->height;
            fb.pitch     = fb_tag->pitch;
            fb.bpp       = fb_tag->bpp;
            fb.available = true;

            uint32_t stride = fb.pitch / 4;
            uint32_t needed = fb.hw_height * stride;
            if (needed <= FB_MAX_PIXELS) {
                fb.backbuffer = backbuf;
                memset(fb.backbuffer, 0, needed * 4);
            }
            return true;
        }
        uint32_t tag_size = (tag->size + 7) & ~7;
        ptr += tag_size;
    }
    return false;
}

void fb_put_pixel(uint32_t x, uint32_t y, uint32_t color)
{
    if (!fb.available || x >= fb.width || y >= fb.height) return;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    target[y * (fb.pitch / 4) + x] = color;
}

uint32_t fb_get_pixel(uint32_t x, uint32_t y)
{
    if (!fb.available || x >= fb.width || y >= fb.height) return 0;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    return target[y * (fb.pitch / 4) + x];
}

void fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    if (!fb.available) return;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    uint32_t stride = fb.pitch / 4;
    for (uint32_t row = y; row < y + h && row < fb.height; row++) {
        uint32_t offset = row * stride + x;
        for (uint32_t col = 0; col < w && (x + col) < fb.width; col++)
            target[offset + col] = color;
    }
}

void fb_clear(uint32_t color)
{
    if (!fb.available) return;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    uint32_t stride = fb.pitch / 4;
    for (uint32_t y = 0; y < fb.height; y++) {
        uint32_t offset = y * stride;
        for (uint32_t x = 0; x < fb.width; x++)
            target[offset + x] = color;
    }
}

void fb_swap(void)
{
    if (!fb.available || !fb.backbuffer) return;
    uint32_t stride = fb.pitch / 4;
    for (uint32_t y = 0; y < fb.height; y++) {
        uint32_t offset = y * stride;
        memcpy(&fb.address[offset], &fb.backbuffer[offset], fb.width * 4);
    }
}

void fb_draw_hline(uint32_t x, uint32_t y, uint32_t w, uint32_t color)
{
    if (!fb.available || y >= fb.height) return;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    uint32_t stride = fb.pitch / 4;
    uint32_t end = x + w;
    if (end > fb.width) end = fb.width;
    uint32_t offset = y * stride;
    for (uint32_t cx = x; cx < end; cx++)
        target[offset + cx] = color;
}

void fb_draw_vline(uint32_t x, uint32_t y, uint32_t h, uint32_t color)
{
    if (!fb.available || x >= fb.width) return;
    uint32_t* target = fb.backbuffer ? fb.backbuffer : fb.address;
    uint32_t stride = fb.pitch / 4;
    uint32_t end = y + h;
    if (end > fb.height) end = fb.height;
    for (uint32_t cy = y; cy < end; cy++)
        target[cy * stride + x] = color;
}

void fb_draw_rect_outline(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color)
{
    fb_draw_hline(x, y, w, color);
    fb_draw_hline(x, y + h - 1, w, color);
    fb_draw_vline(x, y, h, color);
    fb_draw_vline(x + w - 1, y, h, color);
}

void fb_set_virtual_size(uint32_t w, uint32_t h)
{
    if (!fb.available) return;
    if (w > fb.hw_width)  w = fb.hw_width;
    if (h > fb.hw_height) h = fb.hw_height;
    if (w < 320) w = 320;
    if (h < 240) h = 240;

    fb.width  = w;
    fb.height = h;

    /* Tum tamponlari siyaha boya */
    uint32_t stride = fb.pitch / 4;
    uint32_t total = fb.hw_height * stride;
    for (uint32_t i = 0; i < total; i++) {
        fb.address[i] = 0;
        if (fb.backbuffer) fb.backbuffer[i] = 0;
    }
}

uint32_t fb_get_max_width(void)  { return fb.hw_width; }
uint32_t fb_get_max_height(void) { return fb.hw_height; }

framebuffer_t* fb_get_info(void) { return &fb; }
