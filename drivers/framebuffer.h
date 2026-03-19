#ifndef WINTAKOS_FRAMEBUFFER_H
#define WINTAKOS_FRAMEBUFFER_H

#include "../include/types.h"

#define RGB(r, g, b) ((uint32_t)((r) << 16 | (g) << 8 | (b)))

#define COLOR_BLACK       RGB(0, 0, 0)
#define COLOR_WHITE       RGB(255, 255, 255)
#define COLOR_RED         RGB(255, 0, 0)
#define COLOR_GREEN       RGB(0, 200, 0)
#define COLOR_BLUE        RGB(0, 100, 255)
#define COLOR_YELLOW      RGB(255, 255, 0)
#define COLOR_CYAN        RGB(0, 220, 220)
#define COLOR_DARK_GREY   RGB(80, 80, 80)
#define COLOR_LIGHT_GREY  RGB(180, 180, 180)
#define COLOR_ORANGE      RGB(255, 140, 0)
#define COLOR_DARK_BLUE   RGB(20, 20, 60)
#define COLOR_BG_DEFAULT  RGB(30, 30, 50)

typedef struct {
    uint32_t* address;
    uint32_t  width;
    uint32_t  height;
    uint32_t  pitch;
    uint8_t   bpp;
    bool      available;
} framebuffer_t;

bool     fb_init(void* mbi_ptr);
void     fb_put_pixel(uint32_t x, uint32_t y, uint32_t color);
uint32_t fb_get_pixel(uint32_t x, uint32_t y);
void     fb_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);
void     fb_clear(uint32_t color);
void     fb_swap(void);

framebuffer_t* fb_get_info(void);

#endif
