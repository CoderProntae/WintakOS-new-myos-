#include "desktop.h"
#include "window.h"
#include "cursor.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../apps/setup.h"
#include "../lib/string.h"
#include "../include/version.h"

static uint32_t screen_w = 800;
static uint32_t screen_h = 600;

static uint32_t cursor_save_buf[CURSOR_W * CURSOR_H];
static int32_t  last_cx = -1, last_cy = -1;

static uint8_t  last_sec_rtc = 0xFF;
static uint8_t  prev_buttons = 0;
static bool     start_menu_visible = false;
static uint32_t last_update_tick = 0;

static uint32_t theme_bg_top;
static uint32_t theme_bg_bot;
static uint32_t theme_taskbar;
static uint32_t theme_taskbar_text;
static uint32_t theme_btn;

static void apply_theme(uint8_t theme)
{
    switch (theme) {
        case 1: /* Acik */
            theme_bg_top = RGB(200, 215, 240);
            theme_bg_bot = RGB(230, 240, 250);
            theme_taskbar = RGB(230, 230, 238);
            theme_taskbar_text = RGB(30, 30, 40);
            theme_btn = RGB(90, 130, 200);
            break;
        case 2: /* Okyanus */
            theme_bg_top = RGB(5, 30, 80);
            theme_bg_bot = RGB(10, 80, 140);
            theme_taskbar = RGB(5, 20, 60);
            theme_taskbar_text = RGB(180, 220, 255);
            theme_btn = RGB(15, 60, 120);
            break;
        case 3: /* Orman */
            theme_bg_top = RGB(15, 40, 20);
            theme_bg_bot = RGB(30, 80, 40);
            theme_taskbar = RGB(20, 30, 15);
            theme_taskbar_text = RGB(200, 240, 200);
            theme_btn = RGB(30, 80, 40);
            break;
        case 4: /* Gun Batimi */
            theme_bg_top = RGB(40, 20, 60);
            theme_bg_bot = RGB(180, 80, 40);
            theme_taskbar = RGB(50, 20, 30);
            theme_taskbar_text = RGB(255, 220, 180);
            theme_btn = RGB(160, 60, 40);
            break;
        default: /* Koyu */
            theme_bg_top = RGB(15, 20, 50);
            theme_bg_bot = RGB(30, 50, 100);
            theme_taskbar = RGB(20, 20, 35);
            theme_taskbar_text = COLOR_WHITE;
            theme_btn = RGB(45, 80, 160);
            break;
    }
}

static void draw_char_px(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    if (px >= screen_w - 8 || py >= screen_h - 16) return;
    const uint8_t* glyph = font8x16_data[c];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++) {
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
        }
    }
}

static void draw_string_px(uint32_t px, uint32_t py, const char* str, uint32_t fg, uint32_t bg)
{
    while (*str) {
        if (px >= screen_w - 8) break;
        draw_char_px(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static uint32_t gradient_color_at(uint32_t y, uint32_t height)
{
    if (height == 0) height = 1;
    uint32_t ratio = y * 255 / height;
    uint8_t r = (uint8_t)(((theme_bg_top >> 16) & 0xFF) * (255 - ratio) / 255 +
                           ((theme_bg_bot >> 16) & 0xFF) * ratio / 255);
    uint8_t g = (uint8_t)(((theme_bg_top >> 8) & 0xFF) * (255 - ratio) / 255 +
                           ((theme_bg_bot >> 8) & 0xFF) * ratio / 255);
    uint8_t b = (uint8_t)((theme_bg_top & 0xFF) * (255 - ratio) / 255 +
                           (theme_bg_bot & 0xFF) * ratio / 255);
    return RGB(r, g, b);
}

static void draw_background(void)
{
    uint32_t bg_h = screen_h - TASKBAR_HEIGHT;
    for (uint32_t y = 0; y < bg_h; y++) {
        uint32_t color = gradient_color_at(y, bg_h);
        for (uint32_t x = 0; x < screen_w; x++)
            fb_put_pixel(x, y, color);
    }

    setup_config_t* cfg = setup_get_config();
    uint32_t text_bg = gradient_color_at(30, bg_h);

    if (cfg->completed && strlen(cfg->username) > 0) {
        char msg[48];
        uint32_t p = 0;
        const char* prefix = "Ho\x03 geldin, ";
        while (*prefix) msg[p++] = *prefix++;
        const char* u = cfg->username;
        while (*u) msg[p++] = *u++;
        msg[p] = '\0';
        draw_string_px(screen_w / 2 - (p * 8) / 2, 30, msg,
                       RGB(220, 220, 255), text_bg);
    } else {
        draw_string_px(screen_w / 2 - 72, 30, WINTAKOS_FULL,
                       RGB(200, 200, 255), text_bg);
    }
}

static void draw_taskbar(void)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;
    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, theme_taskbar);
    fb_fill_rect(0, ty, screen_w, 1, RGB(60, 60, 100));

    uint32_t btn_c;
    if (start_menu_visible) {
        uint8_t br = (uint8_t)((theme_btn >> 16) & 0xFF);
        uint8_t bg = (uint8_t)((theme_btn >> 8) & 0xFF);
        uint8_t bb = (uint8_t)(theme_btn & 0xFF);
        if (br < 224) br += 32; else br = 255;
        if (bg < 224) bg += 32; else bg = 255;
        if (bb < 224) bb += 32; else bb = 255;
        btn_c = RGB(br, bg, bb);
    } else {
        btn_c = theme_btn;
    }

    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, btn_c);
    draw_string_px(12, ty + 8, "Ba\x03lat", theme_taskbar_text, btn_c);

    rtc_time_t t;
    rtc_read(&t);
    char ts[9];
    ts[0] = '0' + (char)(t.hour / 10);
    ts[1] = '0' + (char)(t.hour % 10);
    ts[2] = ':';
    ts[3] = '0' + (char)(t.minute / 10);
    ts[4] = '0' + (char)(t.minute % 10);
    ts[5] = ':';
    ts[6] = '0' + (char)(t.second / 10);
    ts[7] = '0' + (char)(t.second % 10);
    ts[8] = '\0';
    draw_string_px(screen_w - 80, ty + 8, ts, theme_taskbar_text, theme_taskbar);

    char ds[11];
    ds[0] = '0' + (char)(t.day / 10);
    ds[1] = '0' + (char)(t.day % 10);
    ds[2] = '/';
    ds[3] = '0' + (char)(t.month / 10);
    ds[4] = '0' + (char)(t.month % 10);
    ds[5] = '/';
    ds[6] = '0' + (char)((t.year / 1000) % 10);
    ds[7] = '0' + (char)((t.year / 100) % 10);
    ds[8] = '0' + (char)((t.year / 10) % 10);
    ds[9] = '0' + (char)(t.year % 10);
    ds[10] = '\0';
    draw_string_px(screen_w - 168, ty + 8, ds, RGB(160, 160, 180), theme_taskbar);

    setup_config_t* cfg = setup_get_config();
    if (cfg->completed && strlen(cfg->username) > 0) {
        draw_string_px(100, ty + 8, cfg->username, RGB(160, 180, 220), theme_taskbar);
    }
}

#define START_MENU_W 160
#define START_MENU_H 140

static void draw_start_menu(void)
{
    if (!start_menu_visible) return;
    uint32_t mx = 4;
    uint32_t my = screen_h - TASKBAR_HEIGHT - START_MENU_H;
    uint32_t mbg = RGB(35, 35, 55);

    fb_fill_rect(mx, my, START_MENU_W, START_MENU_H, mbg);
    fb_fill_rect(mx, my, START_MENU_W, 1, RGB(60, 60, 100));
    fb_fill_rect(mx, my, 1, START_MENU_H, RGB(60, 60, 100));
    fb_fill_rect(mx + START_MENU_W - 1, my, 1, START_MENU_H, RGB(60, 60, 100));

    fb_fill_rect(mx + 1, my + 1, START_MENU_W - 2, 24, RGB(40, 80, 170));
    draw_string_px(mx + 8, my + 5, "WintakOS " WINTAKOS_VERSION, COLOR_WHITE, RGB(40, 80, 170));

    draw_string_px(mx + 12, my + 32, "Terminal", COLOR_WHITE, mbg);
    fb_fill_rect(mx + 1, my + 52, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 60, "Sistem Bilgisi", COLOR_WHITE, mbg);
    fb_fill_rect(mx + 1, my + 80, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 88, "Ayarlar", COLOR_WHITE, mbg);
    fb_fill_rect(mx + 1, my + 108, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 116, "Hakk\x01nda", COLOR_LIGHT_GREY, mbg);
}

static void cursor_save_bg(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++)
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x, py = cy + (int32_t)y;
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h)
                cursor_save_buf[y * CURSOR_W + x] = fb_get_pixel((uint32_t)px, (uint32_t)py);
        }
}

static void cursor_restore_bg(int32_t cx, int32_t cy)
{
    if (cx < 0 && cy < 0) return;
    for (uint32_t y = 0; y < CURSOR_H; y++)
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x, py = cy + (int32_t)y;
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h)
                fb_put_pixel((uint32_t)px, (uint32_t)py, cursor_save_buf[y * CURSOR_W + x]);
        }
}

static void cursor_draw(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++)
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x, py = cy + (int32_t)y;
            if (px < 0 || py < 0 || px >= (int32_t)screen_w || py >= (int32_t)screen_h) continue;
            uint8_t v = cursor_bitmap[y][x];
            if (v == 1) fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
            else if (v == 2) fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
        }
}

void desktop_init(void)
{
    framebuffer_t* info = fb_get_info();
    screen_w = info->width;
    screen_h = info->height;

    setup_config_t* cfg = setup_get_config();
    apply_theme(cfg->completed ? cfg->theme : 0);

    draw_background();
    draw_taskbar();
    wm_init();
    last_cx = -1;
    last_cy = -1;
    last_sec_rtc = 0xFF;
    prev_buttons = 0;
    start_menu_visible = false;
    last_update_tick = 0;
}

void desktop_apply_config(void)
{
    setup_config_t* cfg = setup_get_config();
    apply_theme(cfg->theme);
    wm_set_dirty();
}

bool desktop_start_menu_open(void)
{
    return start_menu_visible;
}

int desktop_start_menu_click(int32_t mx, int32_t my)
{
    uint32_t smx = 4;
    uint32_t smy = screen_h - TASKBAR_HEIGHT - START_MENU_H;
    if (mx < (int32_t)smx || mx >= (int32_t)(smx + START_MENU_W)) return -1;
    if (my < (int32_t)smy || my >= (int32_t)(smy + START_MENU_H)) return -1;
    int32_t ry = my - (int32_t)smy;
    if (ry >= 28 && ry < 52) return 0;
    if (ry >= 56 && ry < 80) return 1;
    if (ry >= 84 && ry < 108) return 2;
    if (ry >= 112 && ry < 136) return 3;
    return -1;
}

void desktop_update(void)
{
    uint32_t now = pit_get_ticks();
    if (now - last_update_tick < 3) return;
    last_update_tick = now;

    mouse_state_t ms = mouse_get_state();
    bool mouse_moved = (ms.x != last_cx || ms.y != last_cy);
    bool left_pressed = (ms.buttons & MOUSE_BTN_LEFT) && !(prev_buttons & MOUSE_BTN_LEFT);

    if (left_pressed) {
        uint32_t ty = screen_h - TASKBAR_HEIGHT;
        if (ms.x >= 4 && ms.x < 84 && ms.y >= (int32_t)ty + 4 && ms.y < (int32_t)ty + 28) {
            start_menu_visible = !start_menu_visible;
            wm_set_dirty();
        } else if (start_menu_visible) {
            start_menu_visible = false;
            wm_set_dirty();
        }
    }

    wm_handle_mouse(ms.x, ms.y, ms.buttons);
    prev_buttons = ms.buttons;

    if (wm_is_dirty()) {
        cursor_restore_bg(last_cx, last_cy);
        draw_background();
        wm_draw_all();
        draw_taskbar();
        if (start_menu_visible) draw_start_menu();
        wm_clear_dirty();
        cursor_save_bg(ms.x, ms.y);
        cursor_draw(ms.x, ms.y);
        last_cx = ms.x;
        last_cy = ms.y;
    } else if (mouse_moved) {
        cursor_restore_bg(last_cx, last_cy);
        cursor_save_bg(ms.x, ms.y);
        cursor_draw(ms.x, ms.y);
        last_cx = ms.x;
        last_cy = ms.y;
    } else {
        rtc_time_t t;
        rtc_read(&t);
        if (t.second != last_sec_rtc) {
            last_sec_rtc = t.second;
            cursor_restore_bg(last_cx, last_cy);
            draw_taskbar();
            if (start_menu_visible) draw_start_menu();
            cursor_save_bg(last_cx, last_cy);
            cursor_draw(last_cx, last_cy);
        }
    }
}
