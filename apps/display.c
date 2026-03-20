#include "display.h"
#include "../gui/widget.h"
#include "../gui/desktop.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"

static display_app_t displays[2];
static uint32_t disp_count = 0;

typedef struct { uint32_t w, h; } res_entry_t;

static const res_entry_t all_res[] = {
    {800, 600},
    {640, 480},
    {640, 400},
    {320, 240},
};
#define ALL_RES_COUNT 4

static void uint_to_str_d(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void res_to_str(uint32_t w, uint32_t h, char* buf)
{
    uint32_t p = 0;
    char nb[8];
    uint_to_str_d(w, nb);
    for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
    buf[p++] = 'x';
    uint_to_str_d(h, nb);
    for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
    buf[p] = 0;
}

static display_app_t* find_disp(window_t* win)
{
    for (uint32_t i = 0; i < disp_count; i++)
        if (displays[i].win == win) return &displays[i];
    return NULL;
}

static void disp_draw(window_t* win)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    uint32_t max_w = desktop_get_max_w();
    uint32_t max_h = desktop_get_max_h();
    uint32_t cur_w = desktop_get_screen_w();
    uint32_t cur_h = desktop_get_screen_h();

    char buf[48];
    widget_draw_label(win, 12, 8, "=== Ekran Ayarlar\x01 ===",
                      RGB(100, 200, 255));

    /* Mevcut */
    uint32_t bp = 0;
    const char* pf = "Mevcut:   ";
    while (*pf) buf[bp++] = *pf++;
    char rb[16];
    res_to_str(cur_w, cur_h, rb);
    for (uint32_t i = 0; rb[i]; i++) buf[bp++] = rb[i];
    buf[bp] = 0;
    widget_draw_label(win, 12, 32, buf, RGB(200, 200, 200));

    /* Maksimum */
    bp = 0;
    pf = "Maksimum: ";
    while (*pf) buf[bp++] = *pf++;
    res_to_str(max_w, max_h, rb);
    for (uint32_t i = 0; rb[i]; i++) buf[bp++] = rb[i];
    buf[bp] = 0;
    widget_draw_label(win, 12, 52, buf, RGB(180, 180, 180));

    /* Cozunurluk secenekleri */
    widget_draw_label(win, 12, 80, "\x10\x08z\x07n\x07rl\x07k Se\x0F:",
                      RGB(200, 200, 200));

    for (uint32_t i = 0; i < ALL_RES_COUNT; i++) {
        uint32_t ry = 100 + i * 24;
        char label[16];
        res_to_str(all_res[i].w, all_res[i].h, label);

        bool avail = (all_res[i].w <= max_w && all_res[i].h <= max_h);
        bool sel = (d->selected == i);
        bool current = (all_res[i].w == cur_w && all_res[i].h == cur_h);

        uint32_t color;
        if (!avail)
            color = RGB(100, 100, 100);
        else if (current)
            color = RGB(100, 255, 100);
        else
            color = COLOR_WHITE;

        widget_draw_radio(win, 20, ry, label, sel, color);

        if (current)
            widget_draw_label(win, 140, ry, "(aktif)", RGB(100, 255, 100));
        if (!avail)
            widget_draw_label(win, 140, ry, "(yok)", RGB(100, 100, 100));
    }

    /* Uygula butonu */
    uint32_t btn_y = 100 + ALL_RES_COUNT * 24 + 12;
    widget_draw_button(win, 20, btn_y, 100, 28, "Uygula",
                       RGB(40, 100, 180), COLOR_WHITE);

    /* Hata mesaji */
    if (d->error) {
        widget_draw_label(win, 12, btn_y + 36, d->error_msg,
                          RGB(255, 100, 100));
    }

    /* Onay bekleniyor mesaji */
    if (desktop_is_confirming()) {
        widget_draw_label(win, 12, btn_y + 36, "Onay bekleniyor...",
                          RGB(255, 200, 100));
    }
}

static void disp_click(window_t* win, int32_t rx, int32_t ry)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    uint32_t max_w = desktop_get_max_w();
    uint32_t max_h = desktop_get_max_h();

    /* Radio butonlari */
    for (uint32_t i = 0; i < ALL_RES_COUNT; i++) {
        uint32_t by = 100 + i * 24;
        if (rx >= 20 && rx < 200 && ry >= (int32_t)by && ry < (int32_t)(by + 20)) {
            d->selected = i;
            d->error = false;
            wm_set_dirty();
            return;
        }
    }

    /* Uygula butonu */
    uint32_t btn_y = 100 + ALL_RES_COUNT * 24 + 12;
    if (widget_button_hit(win, 20, btn_y, 100, 28, rx, ry)) {
        uint32_t sel_w = all_res[d->selected].w;
        uint32_t sel_h = all_res[d->selected].h;

        if (sel_w > max_w || sel_h > max_h) {
            d->error = true;
            uint32_t ep = 0;
            const char* em = "Hata: Monit\x08r desteklemiyor!";
            while (*em && ep < 47) d->error_msg[ep++] = *em++;
            d->error_msg[ep] = 0;
            wm_set_dirty();
            return;
        }

        if (sel_w == desktop_get_screen_w() &&
            sel_h == desktop_get_screen_h()) {
            d->error = true;
            uint32_t ep = 0;
            const char* em = "Zaten bu \x0F\x08z\x07n\x07rl\x07kte.";
            while (*em && ep < 47) d->error_msg[ep++] = *em++;
            d->error_msg[ep] = 0;
            wm_set_dirty();
            return;
        }

        d->error = false;
        desktop_set_cursor(CURSOR_BUSY);
        desktop_request_resolution(sel_w, sel_h);
    }
}

display_app_t* display_create(int32_t x, int32_t y)
{
    if (disp_count >= 2) return NULL;
    display_app_t* d = &displays[disp_count++];
    d->selected = 0;
    d->error = false;
    d->error_msg[0] = 0;

    /* Mevcut cozunurlugu sec */
    uint32_t cur_w = desktop_get_screen_w();
    uint32_t cur_h = desktop_get_screen_h();
    for (uint32_t i = 0; i < ALL_RES_COUNT; i++) {
        if (all_res[i].w == cur_w && all_res[i].h == cur_h) {
            d->selected = i;
            break;
        }
    }

    uint32_t wh = 100 + ALL_RES_COUNT * 24 + 80;
    d->win = wm_create_window(x, y, 240, wh, "Ekran Ayarlar\x01",
                               RGB(35, 35, 52));
    if (d->win) {
        d->win->on_draw = disp_draw;
        d->win->on_click = disp_click;
    }
    return d;
}
