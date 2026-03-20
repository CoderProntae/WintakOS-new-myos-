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
    uint32_t    w;
    uint32_t    h;
    const char* label;
} res_entry_t;

static const res_entry_t res_options[] = {
    { 640,  480,  "640x480"   },
    { 800,  600,  "800x600"   },
    { 1024, 768,  "1024x768"  },
    { 1280, 720,  "1280x720"  },
    { 1280, 1024, "1280x1024" },
    { 1920, 1080, "1920x1080" },
};
#define RES_COUNT 6

/* Sabit layout pozisyonlari */
#define L_TITLE_Y       8
#define L_INFO_Y        36
#define L_LINE_H        20
#define L_SEP_Y         140
#define L_SELECT_Y      156
#define L_DROPDOWN_Y    180
#define L_DROPDOWN_H    28
#define L_ITEM_H        24
#define L_APPLY_Y       220
#define L_MSG_Y         258
#define L_GRUB1_Y       280
#define L_GRUB2_Y       300
#define L_GRUB3_Y       322

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

static void draw_char_d(uint32_t px, uint32_t py, uint8_t c,
                         uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    if (px >= fb->width - 8 || py >= fb->height - 16) return;
    const uint8_t* glyph = font8x16_data[c];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++)
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
    }
}

static void draw_str_d(uint32_t px, uint32_t py, const char* s,
                        uint32_t fg, uint32_t bg)
{
    while (*s) {
        draw_char_d(px, py, (uint8_t)*s, fg, bg);
        px += 8;
        s++;
    }
}

static void disp_draw(window_t* win)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    framebuffer_t* fb = fb_get_info();
    int32_t wx = win->x;
    int32_t wy = win->y;
    char buf[64];

    /* ---- Baslik ---- */
    widget_draw_label(win, 12, L_TITLE_Y,
                      "=== Ekran Ayarlar\x01 ===", RGB(100, 200, 255));

    /* ---- Mevcut bilgiler ---- */
    uint32_t y = L_INFO_Y;

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
    y += L_LINE_H;

    /* Renk */
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
    y += L_LINE_H;

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
    y += L_LINE_H;

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
    y += L_LINE_H;

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
    if (wx + 8 >= 0 && wy + (int32_t)L_SEP_Y >= 0)
        fb_fill_rect((uint32_t)(wx + 8), (uint32_t)(wy + (int32_t)L_SEP_Y),
                     win->width - 16, 1, RGB(60, 60, 100));

    /* ---- Secim etiketi ---- */
    widget_draw_label(win, 12, L_SELECT_Y,
                      "\x10\x08z\x07n\x07rl\x07k Se\x0F:",
                      RGB(200, 200, 220));

    /* ---- Dropdown buton ---- */
    {
        uint32_t dbx = 24;
        uint32_t dby = L_DROPDOWN_Y;
        uint32_t dbw = 200;
        uint32_t dbh = L_DROPDOWN_H;

        uint32_t btn_bg = d->dropdown_open ? RGB(50, 70, 120) :
                          RGB(40, 50, 80);

        widget_draw_button(win, dbx, dby, dbw, dbh,
                           res_options[d->selected].label,
                           btn_bg, COLOR_WHITE);

        /* Ok isareti (saga bakan ucgen) */
        int32_t arrow_x = wx + (int32_t)dbx + (int32_t)dbw - 20;
        int32_t arrow_y = wy + (int32_t)dby + 8;
        if (arrow_x > 0 && arrow_y > 0) {
            if (d->dropdown_open) {
                /* Yukari ok */
                draw_char_d((uint32_t)arrow_x, (uint32_t)arrow_y,
                            '^', COLOR_WHITE, btn_bg);
            } else {
                /* Asagi ok */
                draw_char_d((uint32_t)arrow_x, (uint32_t)arrow_y,
                            'v', COLOR_WHITE, btn_bg);
            }
        }
    }

    /* ---- Dropdown listesi (aciksa) ---- */
    if (d->dropdown_open) {
        uint32_t lx = (uint32_t)(wx + 24);
        uint32_t ly = (uint32_t)(wy + (int32_t)L_DROPDOWN_Y +
                                  (int32_t)L_DROPDOWN_H);
        uint32_t lw = 200;
        uint32_t total_h = RES_COUNT * L_ITEM_H;

        /* Liste arka plani */
        if (lx > 0 && ly > 0) {
            /* Golge */
            fb_fill_rect(lx + 2, ly + 2, lw, total_h, RGB(10, 10, 20));
            /* Kutu */
            fb_fill_rect(lx, ly, lw, total_h, RGB(30, 35, 55));
            fb_draw_rect_outline(lx, ly, lw, total_h, RGB(80, 100, 160));

            for (uint32_t i = 0; i < RES_COUNT; i++) {
                uint32_t iy = ly + i * L_ITEM_H;
                bool current = (res_options[i].w == fb->width &&
                                res_options[i].h == fb->height);
                bool sel = (d->selected == i);

                uint32_t item_bg;
                if (sel)
                    item_bg = RGB(50, 80, 160);
                else
                    item_bg = RGB(30, 35, 55);

                fb_fill_rect(lx + 1, iy, lw - 2, L_ITEM_H, item_bg);

                /* Secili ise tik isareti */
                if (sel) {
                    draw_str_d(lx + 4, iy + 4, ">",
                               RGB(100, 200, 255), item_bg);
                }

                draw_str_d(lx + 16, iy + 4,
                           res_options[i].label,
                           COLOR_WHITE, item_bg);

                if (current) {
                    draw_str_d(lx + 120, iy + 4,
                               "(aktif)", RGB(100, 255, 100), item_bg);
                }

                /* Ayirici cizgi */
                if (i < RES_COUNT - 1) {
                    fb_draw_hline(lx + 4, iy + L_ITEM_H - 1,
                                 lw - 8, RGB(50, 55, 80));
                }
            }
        }
    }

    /* ---- Dropdown kapali ise alttaki icerik ---- */
    if (!d->dropdown_open) {
        /* Uygula butonu */
        widget_draw_button(win, 24, L_APPLY_Y, 200, 28,
                           "Kaydet (grub.cfg i\x0Fin)",
                           RGB(40, 100, 180), COLOR_WHITE);

        /* Mesaj */
        if (d->saved) {
            widget_draw_label(win, 12, L_MSG_Y,
                              "Kaydedildi!", RGB(100, 255, 100));
            widget_draw_label(win, 12, L_GRUB1_Y,
                              "grub.cfg'de \x03u sat\x01r\x01 de\x05i\x03tirin:",
                              RGB(200, 200, 220));

            /* grub satiri */
            char grub_line[48];
            uint32_t gp = 0;
            const char* gpf = "set gfxmode=";
            while (*gpf) grub_line[gp++] = *gpf++;
            char rb[16];
            res_to_str(res_options[d->selected].w,
                       res_options[d->selected].h, rb);
            for (uint32_t i = 0; rb[i]; i++) grub_line[gp++] = rb[i];
            gpf = "x32";
            while (*gpf) grub_line[gp++] = *gpf++;
            grub_line[gp] = 0;

            widget_draw_label(win, 20, L_GRUB2_Y, grub_line,
                              RGB(255, 200, 100));

            widget_draw_label(win, 12, L_GRUB3_Y,
                              "Sonra yeniden derleyip boot edin.",
                              RGB(150, 150, 180));
        } else {
            /* Aciklama */
            widget_draw_label(win, 12, L_MSG_Y,
                              "Not: VESA modu boot'ta ayarlan\x01r.",
                              RGB(130, 130, 160));
            widget_draw_label(win, 12, L_GRUB1_Y,
                              "grub.cfg de\x05i\x03tirip yeniden",
                              RGB(130, 130, 160));
            widget_draw_label(win, 12, L_GRUB2_Y,
                              "derlemeniz gerekir.",
                              RGB(130, 130, 160));
        }
    }
}

static void disp_click(window_t* win, int32_t rx, int32_t ry)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    /* ---- Dropdown acikken: liste tiklamasi ---- */
    if (d->dropdown_open) {
        int32_t list_top = (int32_t)L_DROPDOWN_Y + (int32_t)L_DROPDOWN_H;
        int32_t list_bot = list_top + (int32_t)(RES_COUNT * L_ITEM_H);

        if (rx >= 24 && rx < 224 &&
            ry >= list_top && ry < list_bot) {
            /* Bir item secildi */
            uint32_t idx = (uint32_t)(ry - list_top) / L_ITEM_H;
            if (idx < RES_COUNT) {
                d->selected = idx;
                d->saved = false;
            }
        }
        /* Her turlu kapat */
        d->dropdown_open = false;
        wm_set_dirty();
        return;
    }

    /* ---- Dropdown buton tiklamasi ---- */
    if (rx >= 24 && rx < 224 &&
        ry >= (int32_t)L_DROPDOWN_Y &&
        ry < (int32_t)(L_DROPDOWN_Y + L_DROPDOWN_H)) {
        d->dropdown_open = !d->dropdown_open;
        wm_set_dirty();
        return;
    }

    /* ---- Kaydet butonu ---- */
    if (!d->dropdown_open &&
        widget_button_hit(win, 24, L_APPLY_Y, 200, 28, rx, ry)) {

        /* display.cfg'ye yaz */
        char cfg_data[64];
        uint32_t p = 0;

        /* Satir 1: cozunurluk */
        char rb[16];
        res_to_str(res_options[d->selected].w,
                   res_options[d->selected].h, rb);
        for (uint32_t i = 0; rb[i]; i++) cfg_data[p++] = rb[i];
        cfg_data[p++] = '\n';

        /* Satir 2: grub satiri */
        const char* gpf = "set gfxmode=";
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
    d->dropdown_open = false;
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

    d->win = wm_create_window(x, y, 280, 360,
                               "Ekran Ayarlar\x01", RGB(35, 35, 52));
    if (d->win) {
        d->win->on_draw = disp_draw;
        d->win->on_click = disp_click;
    }
    return d;
}
