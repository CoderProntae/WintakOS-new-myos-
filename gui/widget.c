#include "widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"

static void wdraw_char(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    const uint8_t* glyph = font8x16_data[c < 128 ? c : 0];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++) {
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
        }
    }
}

void widget_draw_label(window_t* win, uint32_t rx, uint32_t ry,
                       const char* text, uint32_t color)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    while (*text) {
        wdraw_char((uint32_t)px, (uint32_t)py, (uint8_t)*text, color, win->bg_color);
        px += 8;
        text++;
    }
}

void widget_draw_button(window_t* win, uint32_t rx, uint32_t ry,
                        uint32_t w, uint32_t h, const char* text,
                        uint32_t bg, uint32_t fg)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, bg);
    fb_fill_rect((uint32_t)px, (uint32_t)py, w, 1, RGB(200, 200, 200));
    fb_fill_rect((uint32_t)px, (uint32_t)py, 1, h, RGB(200, 200, 200));
    fb_fill_rect((uint32_t)px, (uint32_t)py + h - 1, w, 1, RGB(60, 60, 60));
    fb_fill_rect((uint32_t)px + w - 1, (uint32_t)py, 1, h, RGB(60, 60, 60));

    uint32_t tlen = 0;
    const char* p = text;
    while (*p++) tlen++;

    int32_t tx = px + (int32_t)((w - tlen * 8) / 2);
    int32_t ty = py + (int32_t)((h - 16) / 2);
    if (tx < 0 || ty < 0) return;

    p = text;
    while (*p) {
        wdraw_char((uint32_t)tx, (uint32_t)ty, (uint8_t)*p, fg, bg);
        tx += 8;
        p++;
    }
}

bool widget_button_hit(window_t* win, uint32_t rx, uint32_t ry,
                       uint32_t w, uint32_t h,
                       int32_t click_rx, int32_t click_ry)
{
    (void)win;
    return (click_rx >= (int32_t)rx && click_rx < (int32_t)(rx + w) &&
            click_ry >= (int32_t)ry && click_ry < (int32_t)(ry + h));
}
