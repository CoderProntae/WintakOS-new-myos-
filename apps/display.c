#include "display.h"
#include "../gui/widget.h"
#include "../gui/desktop.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"
#include "../fs/ramfs.h"

static display_app_t displays[2];
static uint32_t disp_count = 0;

typedef struct { uint32_t w, h; const char* label; } res_entry_t;

static const res_entry_t res_options[] = {
    { 800, 600, "800x600"  },
    { 1024, 768, "1024x768" },
    { 1280, 720, "1280x720" },
    { 640, 480, "640x480"  },
};
#define RES_COUNT 4

static void uint_to_str_d(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
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

    framebuffer_t* fb = fb_get_info();
    char buf[64];
    uint32_t y = 8;

    widget_draw_label(win, 12, y, "=== Ekran Bilgisi ===",
                      RGB(100, 200, 255));
    y += 28;

    /* Mevcut cozunurluk */
    {
        uint32_t p = 0;
        const char* pf = "Mevcut:    ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(fb->width, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        buf[p++] = 'x';
        uint_to_str_d(fb->height, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        buf[p] = 0;
        widget_draw_label(win, 12, y, buf, RGB(100, 255, 100));
    }
    y += 20;

    /* Renk derinligi */
    {
        uint32_t p = 0;
        const char* pf = "Renk:      ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(fb->bpp, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " bit";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
        widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    }
    y += 20;

    /* Pitch */
    {
        uint32_t p = 0;
        const char* pf = "Pitch:     ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(fb->pitch, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " byte/sat\x01r";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
        widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    }
    y += 20;

    /* Framebuffer adresi */
    {
        uint32_t p = 0;
        const char* pf = "FB Adres:  0x";
        while (*pf) buf[p++] = *pf++;
        const char hex[] = "0123456789ABCDEF";
        uint32_t addr = (uint32_t)fb->address;
        for (int i = 28; i >= 0; i -= 4)
            buf[p++] = hex[(addr >> i) & 0xF];
        buf[p] = 0;
        widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    }
    y += 20;

    /* VRAM boyutu */
    {
        uint32_t vram_kb = (fb->pitch * fb->height) / 1024;
        uint32_t p = 0;
        const char* pf = "VRAM:      ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(vram_kb, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " KB";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
        widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    }
    y += 28;

    /* Ayirici cizgi */
    fb_fill_rect(win->x + 12, win->y + (int32_t)y, win->width - 24, 1,
                 RGB(60, 60, 100));
    y += 8;

    /* Boot cozunurluk secimi */
    widget_draw_label(win, 12, y, "Boot \x10\x08z\x07n\x07rl\x07\x05\x07:",
                      RGB(200, 200, 200));
    y += 20;

    widget_draw_label(win, 12, y, "(Yeniden ba\x03latma gerekir)",
                      RGB(150, 150, 170));
    y += 20;

    for (uint32_t i = 0; i < RES_COUNT; i++) {
        bool current = (res_options[i].w == fb->width &&
                        res_options[i].h == fb->height);
        bool sel = (d->selected == i);

        uint32_t color;
        if (current)
            color = RGB(100, 255, 100);
        else
            color = COLOR_WHITE;

        widget_draw_radio(win, 20, y, res_options[i].label, sel, color);

        if (current)
            widget_draw_label(win, 140, y, "(aktif)", RGB(100, 255, 100));

        y += 22;
    }

    y += 8;

    /* Kaydet butonu */
    widget_draw_button(win, 20, y, 120, 28, "Ayar\x01 Kaydet",
                       RGB(40, 100, 180), COLOR_WHITE);
    y += 36;

    /* Config durumu */
    if (ramfs_exists("display.cfg")) {
        char cbuf[64];
        int rd = ramfs_read("display.cfg", cbuf, 63);
        if (rd > 0) {
            cbuf[rd] = '\0';
            uint32_t p = 0;
            const char* pf = "Kaydedilmi\x03: ";
            char line[64];
            while (*pf) line[p++] = *pf++;
            for (uint32_t i = 0; cbuf[i] && cbuf[i] != '\n' && p < 60; i++)
                line[p++] = cbuf[i];
            line[p] = 0;
            widget_draw_label(win, 12, y, line, RGB(180, 200, 255));
        }
    }

    y += 20;
    widget_draw_label(win, 12, y,
                      "Not: grub.cfg'de gfxmode ayarlay\x01n.",
                      RGB(120, 120, 150));
}

static void disp_click(window_t* win, int32_t rx, int32_t ry)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    framebuffer_t* fb = fb_get_info();

    /* Cozunurluk bilgi satirlari: 8+28+20*4+28+8+20+20 = ~200 px sonra radio baslar */
    /* Hesap: y baslangiclari */
    uint32_t radio_start = 8 + 28 + 20 + 20 + 20 + 20 + 28 + 8 + 20 + 20;

    /* Radio butonlari */
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        uint32_t by = radio_start + i * 22;
        if (rx >= 20 && rx < 200 &&
            ry >= (int32_t)by && ry < (int32_t)(by + 20)) {
            d->selected = i;
            wm_set_dirty();
            return;
        }
    }

    /* Kaydet butonu */
    uint32_t btn_y = radio_start + RES_COUNT * 22 + 8;
    if (widget_button_hit(win, 20, btn_y, 120, 28, rx, ry)) {
        /* Config dosyasina yaz */
        char cfg_data[32];
        uint32_t p = 0;
        char nb[8];

        uint_to_str_d(res_options[d->selected].w, nb);
        for (uint32_t i = 0; nb[i]; i++) cfg_data[p++] = nb[i];
        cfg_data[p++] = 'x';
        uint_to_str_d(res_options[d->selected].h, nb);
        for (uint32_t i = 0; nb[i]; i++) cfg_data[p++] = nb[i];
        cfg_data[p++] = '\n';
        cfg_data[p] = 0;

        if (!ramfs_exists("display.cfg"))
            ramfs_create("display.cfg", false);
        ramfs_write("display.cfg", cfg_data, p);

        (void)fb;
        wm_set_dirty();
    }
}

display_app_t* display_create(int32_t x, int32_t y)
{
    if (disp_count >= 2) return NULL;
    display_app_t* d = &displays[disp_count++];
    d->selected = 0;

    /* Mevcut cozunurlugu sec */
    framebuffer_t* fb = fb_get_info();
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        if (res_options[i].w == fb->width &&
            res_options[i].h == fb->height) {
            d->selected = i;
            break;
        }
    }

    d->win = wm_create_window(x, y, 280, 380, "Ekran Ayarlar\x01",
                               RGB(35, 35, 52));
    if (d->win) {
        d->win->on_draw = disp_draw;
        d->win->on_click = disp_click;
    }
    return d;
}
