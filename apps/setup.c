#include "setup.h"
#include "../gui/window.h"
#include "../gui/widget.h"
#include "../gui/desktop.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/keyboard.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"
#include "../include/version.h"

static setup_config_t config;
static window_t* setup_win = NULL;
static uint8_t   setup_step = 0;    /* 0=hosgeldin, 1=kullanici, 2=tema, 3=bitti */
static uint32_t  username_cursor = 0;

static void setup_draw(window_t* win);
static void setup_click(window_t* win, int32_t rx, int32_t ry);

/* Turkceleştirilmiş özel karakter stringleri için makrolar */
/* ş=\x03  ı=\x01  ğ=\x05  ü=\x07  ö=\x0C  ç=\x0F  İ=\x02  Ş=\x04  Ğ=\x06  Ü=\x0B  Ö=\x0E  Ç=\x10 */

static void setup_draw(window_t* win)
{
    uint32_t title_c = RGB(40, 80, 170);
    uint32_t text_c  = RGB(30, 30, 30);
    uint32_t sub_c   = RGB(80, 80, 100);

    switch (setup_step) {
        case 0: /* Hosgeldin */
            widget_draw_label(win, 20, 16,  "WintakOS Kurulum Sihirbaz\x01", title_c);
            widget_draw_label(win, 20, 48,  "WintakOS'a ho\x03 geldiniz!", text_c);
            widget_draw_label(win, 20, 72,  "Bu sihirbaz sisteminizi", text_c);
            widget_draw_label(win, 20, 92,  "yap\x01land\x01racakt\x01r.", text_c);
            widget_draw_label(win, 20, 128, "Ba\x03lamak i\x0Fin 'Devam'a", sub_c);
            widget_draw_label(win, 20, 148, "t\x01klay\x01n.", sub_c);
            widget_draw_button(win, 260, 200, 100, 30, "Devam >", RGB(50, 100, 180), COLOR_WHITE);
            break;

        case 1: /* Kullanici adi */
            widget_draw_label(win, 20, 16,  "Kullan\x01" "c\x01 Ad\x01", title_c);
            widget_draw_label(win, 20, 48,  "L\x07tfen bir kullan\x01" "c\x01 ad\x01", text_c);
            widget_draw_label(win, 20, 68,  "se\x0Finiz:", text_c);
            widget_draw_textbox(win, 20, 100, 240, config.username,
                                username_cursor, true);
            widget_draw_label(win, 20, 140, "(En az 2 karakter)", sub_c);
            widget_draw_button(win, 140, 200, 100, 30, "< Geri", RGB(120, 120, 130), COLOR_WHITE);
            if (strlen(config.username) >= 2) {
                widget_draw_button(win, 260, 200, 100, 30, "Devam >", RGB(50, 100, 180), COLOR_WHITE);
            } else {
                widget_draw_button(win, 260, 200, 100, 30, "Devam >", RGB(150, 150, 160), RGB(200, 200, 200));
            }
            break;

        case 2:
            widget_draw_label(win, 20, 16, "Tema Se\x0Fimi", title_c);
            widget_draw_label(win, 20, 44, "Masa\x07st\x07 temas\x01n\x01 se\x0Fin:", text_c);
            widget_draw_radio(win, 30, 72,  "Koyu (varsay\x01lan)", config.theme == 0, text_c);
            widget_draw_radio(win, 30, 96,  "A\x0F\x01k", config.theme == 1, text_c);
            widget_draw_radio(win, 30, 120, "Okyanus", config.theme == 2, text_c);
            widget_draw_radio(win, 30, 144, "Orman", config.theme == 3, text_c);
            widget_draw_radio(win, 30, 168, "G\x07n Bat\x01m\x01", config.theme == 4, text_c);
            widget_draw_button(win, 140, 200, 100, 30, "< Geri", RGB(120, 120, 130), COLOR_WHITE);
            widget_draw_button(win, 260, 200, 100, 30, "Bitir", RGB(40, 150, 40), COLOR_WHITE);
            break;

        case 3: /* Tamamlandi */
            widget_draw_label(win, 20, 16,  "Kurulum Tamamland\x01!", RGB(40, 150, 40));
            widget_draw_label(win, 20, 52,  "Ho\x03 geldin, ", text_c);

            /* Kullanici adini yaz */
            {
                int32_t px = win->x + 20 + 12 * 8;
                int32_t py = win->y + 52;
                if (px > 0 && py > 0) {
                    const char* u = config.username;
                    while (*u) {
                        /* doğrudan framebuffer'a yaz */
                        const uint8_t* glyph = font8x16_data[(uint8_t)*u];
                        for (uint32_t gy = 0; gy < 16; gy++) {
                            uint8_t line = glyph[gy];
                            for (uint32_t gx = 0; gx < 8; gx++) {
                                fb_put_pixel((uint32_t)px + gx, (uint32_t)py + gy,
                                    (line & (0x80 >> gx)) ? RGB(30, 100, 200) : win->bg_color);
                            }
                        }
                        px += 8;
                        u++;
                    }
                }
            }

            widget_draw_label(win, 20, 88,  "Sisteminiz kullan\x01ma haz\x01r.", text_c);
            widget_draw_label(win, 20, 120, "Masa\x07st\x07n\x07ze y\x0Cnlendiriliyorsunuz...", sub_c);
            widget_draw_progress(win, 20, 160, 340, 16,
                                 (pit_get_ticks() % 300) * 100 / 300,
                                 RGB(50, 120, 200), RGB(200, 200, 210));
            break;
    }
}

static void setup_click(window_t* win, int32_t rx, int32_t ry)
{
    (void)win;

    switch (setup_step) {
        case 0:
            if (widget_button_hit(win, 260, 200, 100, 30, rx, ry)) {
                setup_step = 1;
                wm_set_dirty();
            }
            break;

        case 1:
            if (widget_button_hit(win, 140, 200, 100, 30, rx, ry)) {
                setup_step = 0;
                wm_set_dirty();
            }
            if (strlen(config.username) >= 2 &&
                widget_button_hit(win, 260, 200, 100, 30, rx, ry)) {
                setup_step = 2;
                wm_set_dirty();
            }
            break;

        case 2:
            if (ry >= 72 && ry < 92)   { config.theme = 0; wm_set_dirty(); }
            if (ry >= 96 && ry < 116)  { config.theme = 1; wm_set_dirty(); }
            if (ry >= 120 && ry < 140) { config.theme = 2; wm_set_dirty(); }
            if (ry >= 144 && ry < 164) { config.theme = 3; wm_set_dirty(); }
            if (ry >= 168 && ry < 188) { config.theme = 4; wm_set_dirty(); }

            if (widget_button_hit(win, 140, 200, 100, 30, rx, ry)) {
                setup_step = 1;
                wm_set_dirty();
            }
            if (widget_button_hit(win, 260, 200, 100, 30, rx, ry)) {
                setup_step = 3;
                config.completed = true;

                char cfg_data[128];
                uint32_t p = 0;
                const char* k1 = "username=";
                while (*k1) cfg_data[p++] = *k1++;
                const char* u = config.username;
                while (*u) cfg_data[p++] = *u++;
                cfg_data[p++] = '\n';
                const char* k2 = "theme=";
                while (*k2) cfg_data[p++] = *k2++;
                cfg_data[p++] = '0' + config.theme;
                cfg_data[p++] = '\n';
                const char* k3 = "setup=done\n";
                while (*k3) cfg_data[p++] = *k3++;

                if (!ramfs_exists("config.sys"))
                    ramfs_create("config.sys", false);
                ramfs_write("config.sys", cfg_data, p);

                wm_set_dirty();
            }
            break;

        default:
            break;
    }
}

bool setup_run(void)
{
    /* config.sys var mi kontrol et */
    if (ramfs_exists("config.sys")) {
        char buf[128];
        int rd = ramfs_read("config.sys", buf, 127);
        if (rd > 0) {
            buf[rd] = '\0';
            /* "setup=done" iceriyor mu? */
            for (int i = 0; i < rd - 9; i++) {
                if (buf[i]=='s' && buf[i+1]=='e' && buf[i+2]=='t' &&
                    buf[i+3]=='u' && buf[i+4]=='p' && buf[i+5]=='=' &&
                    buf[i+6]=='d' && buf[i+7]=='o' && buf[i+8]=='n' &&
                    buf[i+9]=='e') {
                    /* Ayarlari yukle */
                    config.completed = true;

                    /* Username parse */
                    for (int j = 0; j < rd - 9; j++) {
                        if (buf[j]=='u' && buf[j+1]=='s' && buf[j+2]=='e' &&
                            buf[j+3]=='r' && buf[j+4]=='n' && buf[j+5]=='a' &&
                            buf[j+6]=='m' && buf[j+7]=='e' && buf[j+8]=='=') {
                            uint32_t k = 0;
                            int s = j + 9;
                            while (s < rd && buf[s] != '\n' && k < SETUP_MAX_USERNAME) {
                                config.username[k++] = buf[s++];
                            }
                            config.username[k] = '\0';
                        }
                    }

                    /* Theme parse */
                    for (int j = 0; j < rd - 6; j++) {
                        if (buf[j]=='t' && buf[j+1]=='h' && buf[j+2]=='e' &&
                            buf[j+3]=='m' && buf[j+4]=='e' && buf[j+5]=='=') {
                            config.theme = (uint8_t)(buf[j+6] - '0');
                            if (config.theme > 4) config.theme = 0;
                        }
                    }

                    return true; /* Kurulum zaten yapilmis */
                }
            }
        }
    }

    /* Ilk kurulum */
    memset(&config, 0, sizeof(config));
    config.theme = 0;
    config.completed = false;
    setup_step = 0;
    username_cursor = 0;

    setup_win = wm_create_window(160, 80, 380, 240, "WintakOS Kurulum", RGB(240, 240, 248));
    if (!setup_win) return false;

    setup_win->on_draw = setup_draw;
    setup_win->on_click = setup_click;
    setup_win->flags &= ~WIN_FLAG_CLOSABLE; /* Kapatilamasin */

    wm_set_dirty();

    /* Kurulum dongusU */
    while (!config.completed) {
        /* Klavye girisi */
        uint8_t c = keyboard_getchar();
        if (c && setup_step == 1) {
            if (c == '\b') {
                if (username_cursor > 0) {
                    username_cursor--;
                    config.username[username_cursor] = '\0';
                    wm_set_dirty();
                }
            }
            else if (c == '\n') {
                if (strlen(config.username) >= 2) {
                    setup_step = 2;
                    wm_set_dirty();
                }
            }
            else if ((c >= 0x20 || (c >= 0x01 && c <= 0x10)) && username_cursor < SETUP_MAX_USERNAME) {
                config.username[username_cursor] = (char)c;
                username_cursor++;
                config.username[username_cursor] = '\0';
                wm_set_dirty();
            }
        }

        desktop_update();
        __asm__ volatile("hlt");
    }

    /* Tamamlandi animasyonu — 3 saniye bekle */
    uint32_t start = pit_get_ticks();
    while (pit_get_ticks() - start < PIT_FREQUENCY * 3) {
        wm_set_dirty();
        desktop_update();
        __asm__ volatile("hlt");
    }

    /* Setup penceresini kapat */
    wm_close_window(setup_win);
    wm_set_dirty();

    return true;
}

setup_config_t* setup_get_config(void)
{
    return &config;
}
