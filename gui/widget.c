#include "widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"

static void wdraw_char(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    if (px >= fb->width - 8 || py >= fb->height - 16) return;
    const uint8_t* glyph = font8x16_data[c];
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
        if ((uint32_t)px >= fb_get_info()->width - 8) break;
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

void widget_draw_textbox(window_t* win, uint32_t rx, uint32_t ry,
                         uint32_t w, const char* text, uint32_t cursor_pos,
                         bool focused)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    uint32_t h = 20;
    uint32_t border = focused ? RGB(80, 140, 220) : RGB(120, 120, 120);

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, COLOR_WHITE);
    fb_fill_rect((uint32_t)px, (uint32_t)py, w, 1, border);
    fb_fill_rect((uint32_t)px, (uint32_t)py + h - 1, w, 1, border);
    fb_fill_rect((uint32_t)px, (uint32_t)py, 1, h, border);
    fb_fill_rect((uint32_t)px + w - 1, (uint32_t)py, 1, h, border);

    int32_t tx = px + 4;
    int32_t ty = py + 2;
    uint32_t i = 0;
    while (text[i] && (uint32_t)(tx - px) < w - 12) {
        wdraw_char((uint32_t)tx, (uint32_t)ty, (uint8_t)text[i], COLOR_BLACK, COLOR_WHITE);
        tx += 8;
        i++;
    }

    if (focused) {
        uint32_t cx = (uint32_t)px + 4 + cursor_pos * 8;
        if (cx < (uint32_t)px + w - 8)
            wdraw_char(cx, (uint32_t)ty, '_', RGB(0, 0, 200), COLOR_WHITE);
    }
}

void widget_draw_radio(window_t* win, uint32_t rx, uint32_t ry,
                       const char* label, bool selected, uint32_t color)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    /* Daire (basit kare olarak) */
    fb_fill_rect((uint32_t)px, (uint32_t)py + 2, 12, 12, RGB(180, 180, 180));
    fb_fill_rect((uint32_t)px + 1, (uint32_t)py + 3, 10, 10, COLOR_WHITE);

    if (selected) {
        fb_fill_rect((uint32_t)px + 3, (uint32_t)py + 5, 6, 6, color);
    }

    int32_t tx = px + 18;
    const char* p = label;
    while (*p) {
        wdraw_char((uint32_t)tx, (uint32_t)py, (uint8_t)*p, color, win->bg_color);
        tx += 8;
        p++;
    }
}

void widget_draw_progress(window_t* win, uint32_t rx, uint32_t ry,
                          uint32_t w, uint32_t h, uint32_t percent,
                          uint32_t fg, uint32_t bg)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, bg);

    uint32_t fill = (w * percent) / 100;
    if (fill > 0) {
        fb_fill_rect((uint32_t)px, (uint32_t)py, fill, h, fg);
    }

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, 1, RGB(100, 100, 100));
    fb_fill_rect((uint32_t)px, (uint32_t)py + h - 1, w, 1, RGB(100, 100, 100));
}
