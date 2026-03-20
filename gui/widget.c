#include "widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"

static void wdraw_char(uint32_t px, uint32_t py, uint8_t c,
                       uint32_t fg, uint32_t bg)
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

static void wdraw_string(uint32_t px, uint32_t py, const char* str,
                          uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    while (*str) {
        if (px >= fb->width - 8) break;
        wdraw_char(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

void widget_draw_label(window_t* win, uint32_t rx, uint32_t ry,
                       const char* text, uint32_t color)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    while (*text) {
        if ((uint32_t)px >= fb_get_info()->width - 8) break;
        wdraw_char((uint32_t)px, (uint32_t)py, (uint8_t)*text,
                   color, win->bg_color);
        px += 8;
        text++;
    }
}

void widget_draw_button(window_t* win, uint32_t rx, uint32_t ry,
                        uint32_t w, uint32_t h, const char* text,
                        uint32_t bg, uint32_t fg)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, bg);
    fb_fill_rect((uint32_t)px, (uint32_t)py, w, 1, RGB(200, 200, 200));
    fb_fill_rect((uint32_t)px, (uint32_t)py, 1, h, RGB(200, 200, 200));
    fb_fill_rect((uint32_t)px, (uint32_t)py + h - 1, w, 1, RGB(60, 60, 60));
    fb_fill_rect((uint32_t)px + w - 1, (uint32_t)py, 1, h, RGB(60, 60, 60));

    uint32_t tlen = strlen(text);
    int32_t tx = px + (int32_t)((w - tlen * 8) / 2);
    int32_t ty = py + (int32_t)((h - 16) / 2);
    if (tx < 0 || ty < 0) return;

    const char* p = text;
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
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    uint32_t h = 20;
    uint32_t border = focused ? RGB(80, 140, 220) : RGB(120, 120, 120);

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, COLOR_WHITE);
    fb_draw_rect_outline((uint32_t)px, (uint32_t)py, w, h, border);

    int32_t tx = px + 4;
    int32_t ty = py + 2;
    uint32_t i = 0;
    while (text[i] && (uint32_t)(tx - px) < w - 12) {
        wdraw_char((uint32_t)tx, (uint32_t)ty, (uint8_t)text[i],
                   COLOR_BLACK, COLOR_WHITE);
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
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    fb_fill_rect((uint32_t)px, (uint32_t)py + 2, 12, 12, RGB(180, 180, 180));
    fb_fill_rect((uint32_t)px + 1, (uint32_t)py + 3, 10, 10, COLOR_WHITE);

    if (selected)
        fb_fill_rect((uint32_t)px + 3, (uint32_t)py + 5, 6, 6, color);

    int32_t tx = px + 18;
    const char* p = label;
    while (*p) {
        wdraw_char((uint32_t)tx, (uint32_t)py, (uint8_t)*p, color,
                   win->bg_color);
        tx += 8;
        p++;
    }
}

void widget_draw_progress(window_t* win, uint32_t rx, uint32_t ry,
                          uint32_t w, uint32_t h, uint32_t percent,
                          uint32_t fg, uint32_t bg)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    fb_fill_rect((uint32_t)px, (uint32_t)py, w, h, bg);

    uint32_t fill = (w * percent) / 100;
    if (fill > 0)
        fb_fill_rect((uint32_t)px, (uint32_t)py, fill, h, fg);

    fb_draw_rect_outline((uint32_t)px, (uint32_t)py, w, h,
                         RGB(100, 100, 100));
}

void widget_draw_checkbox(window_t* win, uint32_t rx, uint32_t ry,
                          const char* label, bool checked, uint32_t color)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;
    int32_t px = win->x + (int32_t)rx;
    int32_t py = win->y + (int32_t)ry;
    if (px < 0 || py < 0) return;

    /* Kutu */
    fb_fill_rect((uint32_t)px, (uint32_t)py + 1, 14, 14, RGB(180, 180, 180));
    fb_fill_rect((uint32_t)px + 1, (uint32_t)py + 2, 12, 12, COLOR_WHITE);

    if (checked) {
        /* Tik isareti — basit X ile */
        uint32_t bx = (uint32_t)px + 3;
        uint32_t by = (uint32_t)py + 3;
        for (uint32_t d = 0; d < 8; d++) {
            fb_put_pixel(bx + d, by + d, color);
            fb_put_pixel(bx + d + 1, by + d, color);
            fb_put_pixel(bx + 7 - d, by + d, color);
            fb_put_pixel(bx + 8 - d, by + d, color);
        }
    }

    int32_t tx = px + 20;
    const char* p = label;
    while (*p) {
        wdraw_char((uint32_t)tx, (uint32_t)py, (uint8_t)*p, color,
                   win->bg_color);
        tx += 8;
        p++;
    }
}

/* ---- Context Menu ---- */

#define CTX_ITEM_H   22
#define CTX_PADDING   4

void ctx_menu_init(context_menu_t* menu)
{
    memset(menu, 0, sizeof(context_menu_t));
    menu->width = 140;
}

void ctx_menu_add(context_menu_t* menu, const char* label)
{
    if (menu->item_count >= CTX_MENU_MAX_ITEMS) return;
    uint32_t i;
    for (i = 0; i < 31 && label[i]; i++)
        menu->items[menu->item_count][i] = label[i];
    menu->items[menu->item_count][i] = '\0';

    uint32_t text_w = (i + 2) * 8;
    if (text_w + CTX_PADDING * 2 > menu->width)
        menu->width = text_w + CTX_PADDING * 2;

    menu->item_count++;
}

void ctx_menu_show(context_menu_t* menu, int32_t x, int32_t y)
{
    framebuffer_t* fb = fb_get_info();
    menu->x = x;
    menu->y = y;

    /* Ekran disina tasmamasi icin */
    uint32_t total_h = menu->item_count * CTX_ITEM_H + CTX_PADDING * 2;
    if (menu->x + (int32_t)menu->width > (int32_t)fb->width)
        menu->x = (int32_t)fb->width - (int32_t)menu->width;
    if (menu->y + (int32_t)total_h > (int32_t)fb->height - 32)
        menu->y = (int32_t)fb->height - 32 - (int32_t)total_h;
    if (menu->x < 0) menu->x = 0;
    if (menu->y < 0) menu->y = 0;

    menu->visible = true;
}

void ctx_menu_hide(context_menu_t* menu)
{
    menu->visible = false;
}

void ctx_menu_draw(context_menu_t* menu)
{
    if (!menu->visible) return;

    uint32_t total_h = menu->item_count * CTX_ITEM_H + CTX_PADDING * 2;
    uint32_t bg = RGB(40, 40, 60);
    uint32_t border = RGB(80, 80, 120);

    fb_fill_rect((uint32_t)menu->x, (uint32_t)menu->y,
                 menu->width, total_h, bg);
    fb_draw_rect_outline((uint32_t)menu->x, (uint32_t)menu->y,
                         menu->width, total_h, border);

    for (uint32_t i = 0; i < menu->item_count; i++) {
        uint32_t iy = (uint32_t)menu->y + CTX_PADDING + i * CTX_ITEM_H;
        wdraw_string((uint32_t)menu->x + CTX_PADDING + 4, iy + 3,
                     menu->items[i], COLOR_WHITE, bg);

        if (i < menu->item_count - 1) {
            fb_draw_hline((uint32_t)menu->x + 4,
                          iy + CTX_ITEM_H - 1,
                          menu->width - 8, RGB(60, 60, 80));
        }
    }
}

int ctx_menu_click(context_menu_t* menu, int32_t mx, int32_t my)
{
    if (!menu->visible) return -1;

    uint32_t total_h = menu->item_count * CTX_ITEM_H + CTX_PADDING * 2;
    if (mx < menu->x || mx >= menu->x + (int32_t)menu->width ||
        my < menu->y || my >= menu->y + (int32_t)total_h) {
        ctx_menu_hide(menu);
        return -1;
    }

    int32_t ry = my - menu->y - (int32_t)CTX_PADDING;
    if (ry < 0) return -1;
    int idx = (int)(ry / CTX_ITEM_H);
    if (idx >= 0 && (uint32_t)idx < menu->item_count) {
        ctx_menu_hide(menu);
        return idx;
    }
    return -1;
}
