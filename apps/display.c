#include "display.h"
#include "../gui/widget.h"
#include "../gui/desktop.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/ata.h"
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
};
#define RES_COUNT 4

/* Layout sabitleri */
#define L_TITLE_Y       8
#define L_INFO_Y        36
#define L_LINE_H        20
#define L_SEP_Y         136
#define L_SELECT_Y      148
#define L_DROPDOWN_Y    168
#define L_DROPDOWN_H    28
#define L_ITEM_H        24
#define L_APPLY_Y       210
#define L_MSG_Y         248
#define L_GRUB_Y        270
#define L_TIP_Y         292

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
    uint32_t p = 0; char nb[8];
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

static void draw_str_abs(uint32_t px, uint32_t py, const char* s,
                          uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    while (*s) {
        if (px >= fb->width - 8) break;
        if (px < fb->width - 8 && py < fb->height - 16) {
            const uint8_t* glyph = font8x16_data[(uint8_t)*s];
            for (uint32_t y = 0; y < 16; y++) {
                uint8_t line = glyph[y];
                for (uint32_t x = 0; x < 8; x++)
                    fb_put_pixel(px + x, py + y,
                                 (line & (0x80 >> x)) ? fg : bg);
            }
        }
        px += 8; s++;
    }
}

static void disp_draw(window_t* win)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    framebuffer_t* fb = fb_get_info();
    char buf[64];
    uint32_t y = L_INFO_Y;

    widget_draw_label(win, 12, L_TITLE_Y,
                      "=== Ekran Ayarlar\x01 ===", RGB(100, 200, 255));

    /* Mevcut */
    {
        uint32_t p = 0; char rb[16];
        const char* pf = "Mevcut:  ";
        while (*pf) buf[p++] = *pf++;
        res_to_str(fb->width, fb->height, rb);
        for (uint32_t i = 0; rb[i]; i++) buf[p++] = rb[i];
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(100, 255, 100));
    y += L_LINE_H;

    /* Renk */
    {
        uint32_t p = 0; char nb[8];
        const char* pf = "Renk:    ";
        while (*pf) buf[p++] = *pf++;
        uint_to_str_d(fb->bpp, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " bit"; while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    y += L_LINE_H;

    /* VRAM */
    {
        uint32_t vram_kb = (fb->pitch * fb->height) / 1024;
        uint32_t p = 0; char nb[8];
        const char* pf = "VRAM:    ";
        while (*pf) buf[p++] = *pf++;
        uint_to_str_d(vram_kb, nb);
        for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
        pf = " KB"; while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf, RGB(200, 200, 200));
    y += L_LINE_H;

    /* Disk durumu */
    {
        uint32_t dc = ata_get_drive_count();
        uint32_t p = 0; char nb[8];
        const char* pf = "Disk:    ";
        while (*pf) buf[p++] = *pf++;
        if (dc == 0) {
            pf = "Bulunamad\x01";
            while (*pf) buf[p++] = *pf++;
        } else {
            uint_to_str_d(dc, nb);
            for (uint32_t i = 0; nb[i]; i++) buf[p++] = nb[i];
            pf = " s\x07r\x07c\x07";
            while (*pf) buf[p++] = *pf++;
        }
        buf[p] = 0;
    }
    widget_draw_label(win, 12, y, buf,
                      ata_get_drive_count() > 0 ?
                      RGB(100, 255, 100) : RGB(255, 100, 100));

    /* Ayirici */
    if (win->x + 8 >= 0 && win->y + (int32_t)L_SEP_Y >= 0)
        fb_fill_rect((uint32_t)(win->x + 8),
                     (uint32_t)(win->y + (int32_t)L_SEP_Y),
                     win->width - 16, 1, RGB(60, 60, 100));

    /* Secim etiketi */
    widget_draw_label(win, 12, L_SELECT_Y,
                      "Boot \x10\x08z\x07n\x07rl\x07\x05\x07:",
                      RGB(200, 200, 220));

    /* Dropdown buton */
    {
        uint32_t dbg = d->dropdown_open ? RGB(50, 70, 120) :
                       RGB(40, 50, 80);
        widget_draw_button(win, 24, L_DROPDOWN_Y, 200, L_DROPDOWN_H,
                           res_options[d->selected].label,
                           dbg, COLOR_WHITE);

        /* Ok isareti */
        int32_t ax = win->x + 24 + 200 - 20;
        int32_t ay = win->y + (int32_t)L_DROPDOWN_Y + 6;
        if (ax > 0 && ay > 0)
            draw_str_abs((uint32_t)ax, (uint32_t)ay,
                         d->dropdown_open ? "^" : "v",
                         COLOR_WHITE, dbg);
    }

    /* Dropdown liste */
    if (d->dropdown_open) {
        uint32_t lx = (uint32_t)(win->x + 24);
        uint32_t ly = (uint32_t)(win->y + (int32_t)L_DROPDOWN_Y +
                                  (int32_t)L_DROPDOWN_H);
        uint32_t lw = 200;
        uint32_t th = RES_COUNT * L_ITEM_H;

        if (lx > 0 && ly > 0) {
            fb_fill_rect(lx + 2, ly + 2, lw, th, RGB(10, 10, 20));
            fb_fill_rect(lx, ly, lw, th, RGB(30, 35, 55));
            fb_draw_rect_outline(lx, ly, lw, th, RGB(80, 100, 160));

            for (uint32_t i = 0; i < RES_COUNT; i++) {
                uint32_t iy = ly + i * L_ITEM_H;
                bool cur = (res_options[i].w == fb->width &&
                            res_options[i].h == fb->height);
                bool sel = (d->selected == i);
                uint32_t ibg = sel ? RGB(50, 80, 160) : RGB(30, 35, 55);

                fb_fill_rect(lx + 1, iy, lw - 2, L_ITEM_H, ibg);
                if (sel)
                    draw_str_abs(lx + 4, iy + 4, ">",
                                 RGB(100, 200, 255), ibg);
                draw_str_abs(lx + 16, iy + 4,
                             res_options[i].label, COLOR_WHITE, ibg);
                if (cur)
                    draw_str_abs(lx + 110, iy + 4,
                                 "(aktif)", RGB(100, 255, 100), ibg);
                if (i < RES_COUNT - 1)
                    fb_draw_hline(lx + 4, iy + L_ITEM_H - 1,
                                 lw - 8, RGB(50, 55, 80));
            }
        }
        return; /* dropdown acikken alt kisimlari cizme */
    }

    /* Kaydet butonu */
    bool has_disk = ata_get_drive_count() > 0;
    uint32_t btn_bg = has_disk ? RGB(40, 100, 180) : RGB(60, 60, 80);
    widget_draw_button(win, 24, L_APPLY_Y, 200, 28,
                       "Diske Kaydet & Uygula",
                       btn_bg, COLOR_WHITE);

    if (!has_disk) {
        widget_draw_label(win, 12, L_MSG_Y,
                          "Disk yok! Kaydetme m\x07mk\x07n de\x05il.",
                          RGB(255, 100, 100));
        widget_draw_label(win, 12, L_GRUB_Y,
                          "GRUB men\x07s\x07nden se\x0Fin.",
                          RGB(150, 150, 180));
        return;
    }

    if (d->saved) {
        widget_draw_label(win, 12, L_MSG_Y,
                          "Diske kaydedildi!", RGB(100, 255, 100));
        /* grub talimat */
        char gl[48]; uint32_t gp = 0;
        const char* gpf = "GRUB'da se\x0Fin: WintakOS ";
        while (*gpf) gl[gp++] = *gpf++;
        char rb[16];
        res_to_str(res_options[d->selected].w,
                   res_options[d->selected].h, rb);
        for (uint32_t i = 0; rb[i]; i++) gl[gp++] = rb[i];
        gl[gp] = 0;
        widget_draw_label(win, 12, L_GRUB_Y, gl, RGB(255, 200, 100));
        widget_draw_label(win, 12, L_TIP_Y,
                          "Yeniden ba\x03lat\x01n.",
                          RGB(150, 150, 180));
    } else {
        widget_draw_label(win, 12, L_MSG_Y,
                          "Se\x0Fip 'Kaydet' bas\x01n.",
                          RGB(130, 130, 160));
    }
}

static void disp_click(window_t* win, int32_t rx, int32_t ry)
{
    display_app_t* d = find_disp(win);
    if (!d) return;

    /* Dropdown acikken */
    if (d->dropdown_open) {
        int32_t lt = (int32_t)L_DROPDOWN_Y + (int32_t)L_DROPDOWN_H;
        int32_t lb = lt + (int32_t)(RES_COUNT * L_ITEM_H);

        if (rx >= 24 && rx < 224 && ry >= lt && ry < lb) {
            uint32_t idx = (uint32_t)(ry - lt) / L_ITEM_H;
            if (idx < RES_COUNT) {
                d->selected = idx;
                d->saved = false;
            }
        }
        d->dropdown_open = false;
        wm_set_dirty();
        return;
    }

    /* Dropdown buton */
    if (rx >= 24 && rx < 224 &&
        ry >= (int32_t)L_DROPDOWN_Y &&
        ry < (int32_t)(L_DROPDOWN_Y + L_DROPDOWN_H)) {
        d->dropdown_open = true;
        wm_set_dirty();
        return;
    }

    /* Kaydet butonu */
    if (widget_button_hit(win, 24, L_APPLY_Y, 200, 28, rx, ry)) {
        if (ata_get_drive_count() == 0) return;

        disk_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.magic = CONFIG_MAGIC;
        cfg.version = CONFIG_VERSION;
        cfg.pref_res_w = res_options[d->selected].w;
        cfg.pref_res_h = res_options[d->selected].h;

        /* Mevcut setup bilgilerini de kaydet */
        setup_config_t* scfg = setup_get_config();
        cfg.theme = scfg->theme;
        uint32_t ui = 0;
        while (ui < 31 && scfg->username[ui]) {
            cfg.username[ui] = scfg->username[ui];
            ui++;
        }
        cfg.username[ui] = '\0';

        if (disk_config_save(&cfg)) {
            d->saved = true;
        }
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

    framebuffer_t* fb = fb_get_info();
    for (uint32_t i = 0; i < RES_COUNT; i++) {
        if (res_options[i].w == fb->width &&
            res_options[i].h == fb->height) {
            d->selected = i;
            break;
        }
    }

    d->win = wm_create_window(x, y, 280, 320,
                               "Ekran Ayarlar\x01", RGB(35, 35, 52));
    if (d->win) {
        d->win->on_draw = disp_draw;
        d->win->on_click = disp_click;
    }
    return d;
}
