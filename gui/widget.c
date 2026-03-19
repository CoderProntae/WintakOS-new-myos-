#include "widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"

static void draw_char_px(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
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
    uint32_t px = (uint32_t)win->x + rx;
    uint32_t py = (uint32_t)win->y + ry;

    while (*text) {
        draw_char_px(px, py, (uint8_t)*text, color, win->bg_color);
        px += 8;
        text++;
    }
}

void widget_draw_button(window_t* win, uint32_t rx, uint32_t ry,
                        uint32_t w, uint32_t h, const char* text,
                        uint32_t bg, uint32_t fg)
{
    uint32_t px = (uint32_t)win->x + rx;
    uint32_t py = (uint32_t)win->y + ry;

    fb_fill_rect(px, py, w, h, bg);

    /* Kenar */
    fb_fill_rect(px, py, w, 1, RGB(200, 200, 200));
    fb_fill_rect(px, py, 1, h, RGB(200, 200, 200));
    fb_fill_rect(px, py + h - 1, w, 1, RGB(60, 60, 60));
    fb_fill_rect(px + w - 1, py, 1, h, RGB(60, 60, 60));

    /* Metin ortala */
    uint32_t text_len = 0;
    const char* p = text;
    while (*p++) text_len++;

    uint32_t tx = px + (w - text_len * 8) / 2;
    uint32_t ty = py + (h - 16) / 2;

    p = text;
    while (*p) {
        draw_char_px(tx, ty, (uint8_t)*p, fg, bg);
        tx += 8;
        p++;
    }
}

bool widget_button_clicked(window_t* win, uint32_t rx, uint32_t ry,
                           uint32_t w, uint32_t h,
                           int32_t mx, int32_t my)
{
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;

    return (mx >= px && mx < px + (int32_t)w &&
            my >= py && my < py + (int32_t)h);
}
