#include "display.h"
#include "../gui/widget.h"
#include "../gui/desktop.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"
#include "../fs/ramfs.h"

static display_app_t displays[2];
static uint32_t disp_count = 0;

typedef struct {
    uint32_t w;
    uint32_t h;
    const char* label;
} res_entry_t;

static const res_entry_t res_options[] = {
    { 640,  480,  "640x480"  },
    { 800,  600,  "800x600"  },
    { 1024, 768,  "1024x768" },
    { 1280, 720,  "1280x720" },
};
#define RES_COUNT 4

/* Sabit y pozisyonlari — draw ve click AYNI degerleri kullanir */
#define TITLE_Y       8
#define INFO_START_Y  36
#define INFO_LINE_H   20
#define SEP_Y         140
#define RADIO_TITLE_Y 152
#define RADIO_NOTE_Y  172
#define RADIO_START_Y 196
#define RADIO_H       26
#define BTN_Y         (RADIO_START_Y + RES_COUNT * RADIO_H + 12)
#define MSG_Y         (BTN_Y + 36)
#define GRUB_Y        (MSG_Y + 22)

static void uint_to_str_d(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12];
    uint32_t tp = 0;
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

    framebuffer_t* fb = fb_get_info();
    char buf[64];

    /* ---- Baslik ---- */
    widget_draw_label(win, 12, TITLE_Y,
                      "=== Ekran Bilgisi ===", RGB(100, 200, 255));

    /* ---- Mevcut bilgiler ---- */
    uint32_t y = INFO_START_Y;

    /* Cozunurluk */
    {
        uint32_t p = 0;
        const char* pf = "Mevcut:  ";
        while (*pf) buf[p++] = *pf++;
        char rb[16];
        res_to_str(fb->width, fb->height, rb);
        for (uint32_t i = 0; rb[i]; i++) buf[p++] = rb[i];
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(100, 255, 100));
    y += INFO_LINE_H;

    /* Renk derinligi */
    {
        uint32_t p = 0;
        const char* pf = "Renk:    ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(fb->bpp, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " bit";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    y += INFO_LINE_H;

    /* Pitch */
    {
        uint32_t p = 0;
        const char* pf = "Pitch:   ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(fb->pitch, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " B/sat\x01r";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    y += INFO_LINE_H;

    /* VRAM */
    {
        uint32_t vram_kb = (fb->pitch * fb->height) / 1024;
        uint32_t p = 0;
        const char* pf = "VRAM:    ";
        while (*pf) buf[p++] = *pf++;
        char nb[8];
        uint_to_str_d(vram_kb, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " KB";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    y += INFO_LINE_H;

    /* FB adresi */
    {
        uint32_t p = 0;
        const char* pf = "Adres:   0x";
        while (*pf) buf[p++] = *pf++;
        const char hex[] = "0123456789ABCDEF";
        uint32_t addr = (uint32_t)fb->address;
        for (int i = 28; i >= 0; i -= 4)
            buf[p++] = hex[(addr >> i) & 0xF];
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));

    /* ---- Ayirici ---- */
    /* safe_fill_rect yerine widget kullan */
    {
        int32_t sx = win->x + 8;
        int32_t sy = win->y + (int32_t)SEP_Y;
        if (sx >= 0 && sy >= 0)
            fb_fill_rect((uint32_t)sx, (uint32_t)sy,
                         win->width - 16, 1, RGB(60, 60, 100));
    }

    /* ---- Boot cozunurluk secimi ---- */
    widget_draw_label(win, 12, RADIO_TITLE_Y,
                      "Yeniden Ba\x03latma \x10\x08z\x07n\x07rl\x07\x05\x07:",
                      RGB(200, 200, 220));

    widget_draw_label(win, 12, RADIO_NOTE_Y,
                      "(grub.cfg'de uygulan\x01r)",
                      RGB(130, 130, 160));

    /* ---- Radio butonlari ---- */
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        uint32_t ry = RADIO_START_Y + i * RADIO_H;
        bool sel = (d->selected == i);
        bool current = (res_options[i].w == fb->width &&
                        res_options[i].h == fb->height);

        uint32_t color = current ? RGB(100, 255, 100) : COLOR_WHITE;

        widget_draw_radio(win, 24, ry, res_options[i].label, sel, color);

        if (current) {
            widget_draw_label(win, 160, ry,
                              "(aktif)", RGB(100, 255, 100));
        }
    }

    /* ---- Kaydet butonu ---- */
    widget_draw_button(win, 24, BTN_Y, 130, 28, "Kaydet + Uygula",
                       RGB(40, 100, 180), COLOR_WHITE);

    /* ---- Mesaj alani ---- */
    if (d->saved) {
        widget_draw_label(win, 12, MSG_Y,
                          "Kaydedildi! Yeniden ba\x03lat\x01n.",
                          RGB(100, 255, 100));

        /* grub.cfg talimatini goster */
        char grub_line[48];
        uint32_t gp = 0;
        const char* gpf = "gfxmode=";
        while (*gpf) grub_line[gp++] = *gpf++;
        char rb[16];
        res_to_str(res_options[d->selected].w,
                   res_options[d->selected].h, rb);
        for (uint32_t i = 0; rb[i]; i++) grub_line[gp++] = rb[i];
        gpf = "x32";
        while (*gpf) grub_line[gp++] = *gpf++;
        grub_line[gp] = 0;

        widget_draw_label(win, 12, GRUB_Y, grub_line,
                          RGB(255, 200, 100));
    }
}

static void disp_click(window_t* win, int32_t rx, int32_t ry)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    /* ---- Radio butonlari ---- */
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        int32_t by = (int32_t)(RADIO_START_Y + i * RADIO_H);
        if (rx >= 16 && rx < 220 &&
            ry >= by && ry < by + (int32_t)RADIO_H) {
            d->selected = i;
            d->saved = false;
            wm_set_dirty();
            return;
        }
    }

    /* ---- Kaydet butonu ---- */
    if (widget_button_hit(win, 24, BTN_Y, 130, 28, rx, ry)) {
        /* Config dosyasina yaz */
        char cfg_data[48];
        uint32_t p = 0;

        /* Cozunurluk */
        char rb[16];
        res_to_str(res_options[d->selected].w,
                   res_options[d->selected].h, rb);
        for (uint32_t i = 0; rb[i]; i++) cfg_data[p++] = rb[i];
        cfg_data[p++] = '\n';

        /* grub.cfg satiri */
        const char* gpf = "gfxmode=";
        while (*gpf) cfg_data[p++] = *gpf++;
        for (uint32_t i = 0; rb[i]; i++) cfg_data[p++] = rb[i];
        gpf = "x32\n";
        while (*gpf) cfg_data[p++] = *gpf++;
        cfg_data[p] = 0;

        if (!ramfs_exists("display.cfg"))
            ramfs_create("display.cfg", false);
        ramfs_write("display.cfg", cfg_data, p);

        d->saved = true;
        wm_set_dirty();
    }
}

display_app_t* display_create(int32_t x, int32_t y)
{
    if (disp_count >= 2) return NULL;
    display_app_t* d = &displays[disp_count++];
    d->selected = 0;
    d->saved = false;

    /* Mevcut cozunurlugu sec */
    framebuffer_t* fb = fb_get_info();
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        if (res_options[i].w == fb->width &&
            res_options[i].h == fb->height) {
            d->selected = i;
            break;
        }
    }

    uint32_t wh = GRUB_Y + 40;
    d->win = wm_create_window(x, y, 280, wh,
                               "Ekran Ayarlar\x01", RGB(35, 35, 52));
    if (d->win) {
        d->win->on_draw = disp_draw;
        d->win->on_click = disp_click;
    }
    return d;
}
