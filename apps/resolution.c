#include "resolution.h"
#include "../gui/widget.h"
#include "../gui/window.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

typedef struct {
    uint32_t w, h;
    char     label[16];
    bool     available;
} res_opt_t;

#define RES_COUNT 6
static res_opt_t res_opts[RES_COUNT] = {
    { 640,  480,  "640x480",   false },
    { 800,  600,  "800x600",   false },
    { 1024, 768,  "1024x768",  false },
    { 1280, 720,  "1280x720",  false },
    { 1280, 1024, "1280x1024", false },
    { 1920, 1080, "1920x1080", false },
};

#define MAX_RES_APPS 2
static resolution_app_t res_apps[MAX_RES_APPS];
static uint32_t res_app_count = 0;
static resolution_app_t* active_app = NULL;

static void uint_to_str_r(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

/* Ana pencere cizimi */
static void res_draw(window_t* win)
{
    resolution_app_t* app = NULL;
    for (uint32_t i = 0; i < res_app_count; i++)
        if (res_apps[i].win == win) { app = &res_apps[i]; break; }
    if (!app) return;

    uint32_t y = 8;

    /* Baslik */
    widget_draw_label(win, 12, y, "=== \x10\x0Cz\x07n\x07rl\x07k ===",
                      RGB(100, 200, 255));
    y += 24;

    /* Mevcut ve max bilgisi */
    framebuffer_t* fb = fb_get_info();
    char buf[48]; uint32_t p;

    p = 0;
    const char* pf = "Mevcut: ";
    while (*pf) buf[p++] = *pf++;
    char nb[12];
    uint_to_str_r(fb->width, nb);
    for (uint32_t j = 0; nb[j]; j++) buf[p++] = nb[j];
    buf[p++] = 'x';
    uint_to_str_r(fb->height, nb);
    for (uint32_t j = 0; nb[j]; j++) buf[p++] = nb[j];
    buf[p] = 0;
    widget_draw_label(win, 12, y, buf, RGB(180, 220, 255));
    y += 18;

    p = 0;
    pf = "Maks:   ";
    while (*pf) buf[p++] = *pf++;
    uint_to_str_r(fb_get_max_width(), nb);
    for (uint32_t j = 0; nb[j]; j++) buf[p++] = nb[j];
    buf[p++] = 'x';
    uint_to_str_r(fb_get_max_height(), nb);
    for (uint32_t j = 0; nb[j]; j++) buf[p++] = nb[j];
    buf[p] = 0;
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 24;

    /* Cozunurluk listesi */
    for (int i = 0; i < RES_COUNT; i++) {
        uint32_t color;
        if (!res_opts[i].available)
            color = RGB(100, 100, 100);
        else if (i == app->selected)
            color = RGB(100, 255, 100);
        else
            color = RGB(200, 200, 200);

        bool sel = (i == app->selected);
        widget_draw_radio(win, 12, y, res_opts[i].label, sel, color);

        if (!res_opts[i].available) {
            widget_draw_label(win, 140, y, "(yok)", RGB(100, 100, 100));
        }

        y += 22;
    }

    y += 8;

    /* Butonlar */
    if (!app->confirming) {
        widget_draw_button(win, 12, y, 100, 28, "Uygula",
                           RGB(40, 120, 40), COLOR_WHITE);
    } else {
        widget_draw_label(win, 12, y, "Onay bekleniyor...",
                          RGB(255, 200, 100));
    }

    /* Hata mesaji */
    if (app->error_msg[0]) {
        widget_draw_label(win, 12, y + 34, app->error_msg,
                          RGB(255, 80, 80));
    }
}

/* Ana pencere tiklama */
static void res_click(window_t* win, int32_t rx, int32_t ry)
{
    resolution_app_t* app = NULL;
    for (uint32_t i = 0; i < res_app_count; i++)
        if (res_apps[i].win == win) { app = &res_apps[i]; break; }
    if (!app || app->confirming) return;

    /* Radio butonlari (y = 74 + i*22) */
    int32_t list_start = 74;
    for (int i = 0; i < RES_COUNT; i++) {
        int32_t iy = list_start + i * 22;
        if (ry >= iy && ry < iy + 20 && rx >= 12 && rx < 200) {
            app->selected = i;
            app->error_msg[0] = '\0';
            wm_set_dirty();
            return;
        }
    }

    /* Uygula butonu */
    int32_t btn_y = list_start + RES_COUNT * 22 + 8;
    if (widget_button_hit(win, 12, (uint32_t)btn_y, 100, 28, rx, ry)) {
        if (app->selected < 0 || app->selected >= RES_COUNT) return;

        if (!res_opts[app->selected].available) {
            /* Hata: desteklenmiyor */
            uint32_t ep = 0;
            const char* em = "Hata! Maks a\x03\x01ld\x01.";
            while (*em) app->error_msg[ep++] = *em++;
            app->error_msg[ep] = '\0';
            wm_set_dirty();
            return;
        }

        framebuffer_t* fb = fb_get_info();
        uint32_t new_w = res_opts[app->selected].w;
        uint32_t new_h = res_opts[app->selected].h;

        /* Ayni cozunurluk secildiyse */
        if (new_w == fb->width && new_h == fb->height) {
            uint32_t ep = 0;
            const char* em = "Zaten bu \x0F\x0Cz\x07n\x07rl\x07kte.";
            while (*em) app->error_msg[ep++] = *em++;
            app->error_msg[ep] = '\0';
            wm_set_dirty();
            return;
        }

        /* Eski cozunurlugu kaydet */
        app->old_w = fb->width;
        app->old_h = fb->height;

        /* Yeni cozunurlugu uygula */
        fb_set_virtual_size(new_w, new_h);

        /* Onay penceresi olustur */
        app->confirming = true;
        app->confirm_start = pit_get_ticks();
        app->error_msg[0] = '\0';
        active_app = app;

        int32_t cx = (int32_t)new_w / 2 - 120;
        int32_t cy = (int32_t)new_h / 2 - 60;
        if (cx < 20) cx = 20;
        if (cy < 30) cy = 30;

        extern void res_confirm_draw(window_t* w);
        extern void res_confirm_click(window_t* w, int32_t rx2, int32_t ry2);

        app->confirm_win = wm_create_window(cx, cy, 240, 100,
                                             "Onay", RGB(40, 40, 60));
        if (app->confirm_win) {
            app->confirm_win->on_draw  = res_confirm_draw;
            app->confirm_win->on_click = res_confirm_click;
            /* Kapatma butonu: iptal gibi davransin */
        }

        wm_set_dirty();
    }
}

/* Onay penceresi cizimi */
void res_confirm_draw(window_t* win)
{
    if (!active_app || !active_app->confirming) return;

    uint32_t elapsed = (pit_get_ticks() - active_app->confirm_start);
    uint32_t secs = elapsed / PIT_FREQUENCY;
    uint32_t remaining = secs >= 7 ? 0 : 7 - secs;

    widget_draw_label(win, 10, 8, "\x10\x0Cz\x07n\x07rl\x07k de\x05i\x03ti.",
                      COLOR_WHITE);

    char buf[32]; uint32_t p = 0;
    const char* pf = "Kalan: ";
    while (*pf) buf[p++] = *pf++;
    buf[p++] = '0' + (char)remaining;
    pf = " saniye";
    while (*pf) buf[p++] = *pf++;
    buf[p] = 0;
    widget_draw_label(win, 10, 30, buf, RGB(255, 200, 100));

    widget_draw_button(win, 15, 58, 90, 28, "Evet",
                       RGB(40, 150, 40), COLOR_WHITE);
    widget_draw_button(win, 130, 58, 90, 28, "Hay\x01r",
                       RGB(180, 40, 40), COLOR_WHITE);
}

/* Onay penceresi tiklama */
void res_confirm_click(window_t* win, int32_t rx, int32_t ry)
{
    (void)win;
    if (!active_app || !active_app->confirming) return;

    /* Evet */
    if (widget_button_hit(win, 15, 58, 90, 28, rx, ry)) {
        active_app->confirming = false;
        wm_close_window(active_app->confirm_win);
        active_app->confirm_win = NULL;
        active_app = NULL;
        wm_set_dirty();
    }
    /* Hayir */
    else if (widget_button_hit(win, 130, 58, 90, 28, rx, ry)) {
        fb_set_virtual_size(active_app->old_w, active_app->old_h);
        active_app->confirming = false;
        wm_close_window(active_app->confirm_win);
        active_app->confirm_win = NULL;
        active_app = NULL;
        wm_set_dirty();
    }
}

/* Her frame cagrilir — timeout kontrolu */
void resolution_update(void)
{
    if (!active_app || !active_app->confirming) return;

    uint32_t elapsed = (pit_get_ticks() - active_app->confirm_start);
    uint32_t secs = elapsed / PIT_FREQUENCY;

    /* 7 saniye doldu — geri al */
    if (secs >= 7) {
        fb_set_virtual_size(active_app->old_w, active_app->old_h);
        active_app->confirming = false;
        if (active_app->confirm_win)
            wm_close_window(active_app->confirm_win);
        active_app->confirm_win = NULL;
        active_app = NULL;
        wm_set_dirty();
        return;
    }

    /* Onay penceresi kapatildiysa da geri al */
    if (active_app->confirm_win &&
        !(active_app->confirm_win->flags & WIN_FLAG_VISIBLE)) {
        fb_set_virtual_size(active_app->old_w, active_app->old_h);
        active_app->confirming = false;
        active_app->confirm_win = NULL;
        active_app = NULL;
        wm_set_dirty();
        return;
    }

    /* Sayac animasyonu icin surekli yeniden ciz */
    wm_set_dirty();
}

resolution_app_t* resolution_create(int32_t x, int32_t y)
{
    if (res_app_count >= MAX_RES_APPS) return NULL;
    resolution_app_t* app = &res_apps[res_app_count++];

    /* Hangi cozunurlukler mevcut */
    uint32_t max_w = fb_get_max_width();
    uint32_t max_h = fb_get_max_height();
    for (int i = 0; i < RES_COUNT; i++)
        res_opts[i].available = (res_opts[i].w <= max_w &&
                                 res_opts[i].h <= max_h);

    /* Mevcut cozunurlugu bul */
    framebuffer_t* fb = fb_get_info();
    app->selected = -1;
    for (int i = 0; i < RES_COUNT; i++) {
        if (res_opts[i].w == fb->width && res_opts[i].h == fb->height) {
            app->selected = i;
            break;
        }
    }

    app->confirming = false;
    app->confirm_win = NULL;
    app->error_msg[0] = '\0';
    app->old_w = fb->width;
    app->old_h = fb->height;

    app->win = wm_create_window(x, y, 240, 280,
                                 "\x10\x0Cz\x07n\x07rl\x07k",
                                 RGB(30, 30, 48));
    if (app->win) {
        app->win->on_draw  = res_draw;
        app->win->on_click = res_click;
    }
    return app;
}
