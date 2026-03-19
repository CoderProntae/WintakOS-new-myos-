#include "desktop.h"
#include "window.h"
#include "cursor.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../lib/string.h"

static uint32_t screen_w = 800;
static uint32_t screen_h = 600;

static uint32_t cursor_save_buf[CURSOR_W * CURSOR_H];
static int32_t  last_cx = -1;
static int32_t  last_cy = -1;

static uint8_t  last_sec_rtc = 0xFF;
static uint8_t  prev_buttons = 0;
static bool     start_menu_visible = false;

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

static void draw_string_px(uint32_t px, uint32_t py, const char* str, uint32_t fg, uint32_t bg)
{
    while (*str) {
        draw_char_px(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static void draw_background(void)
{
    uint32_t bg_h = screen_h - TASKBAR_HEIGHT;
    for (uint32_t y = 0; y < bg_h; y++) {
        uint8_t r = (uint8_t)(20 + y * 30 / bg_h);
        uint8_t g = (uint8_t)(30 + y * 40 / bg_h);
        uint8_t b = (uint8_t)(80 + y * 80 / bg_h);
        uint32_t color = RGB(r, g, b);
        for (uint32_t x = 0; x < screen_w; x++) {
            fb_put_pixel(x, y, color);
        }
    }
    draw_string_px(screen_w / 2 - 60, 30, "WintakOS v0.6.0",
                   RGB(200, 200, 255), RGB(22, 32, 84));
}

static void draw_taskbar(void)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;

    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, RGB(25, 25, 40));
    fb_fill_rect(0, ty, screen_w, 1, RGB(60, 60, 100));

    /* Start button */
    uint32_t btn_color = start_menu_visible ? RGB(70, 110, 200) : RGB(50, 90, 170);
    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, btn_color);
    draw_string_px(12, ty + 8, "WintakOS", COLOR_WHITE, btn_color);

    /* RTC saat */
    rtc_time_t t;
    rtc_read(&t);

    char time_str[9];
    time_str[0] = '0' + (char)(t.hour / 10);
    time_str[1] = '0' + (char)(t.hour % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (char)(t.minute / 10);
    time_str[4] = '0' + (char)(t.minute % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (char)(t.second / 10);
    time_str[7] = '0' + (char)(t.second % 10);
    time_str[8] = '\0';

    draw_string_px(screen_w - 80, ty + 8, time_str, COLOR_WHITE, RGB(25, 25, 40));

    /* Tarih */
    char date_str[11];
    date_str[0] = '0' + (char)(t.day / 10);
    date_str[1] = '0' + (char)(t.day % 10);
    date_str[2] = '/';
    date_str[3] = '0' + (char)(t.month / 10);
    date_str[4] = '0' + (char)(t.month % 10);
    date_str[5] = '/';
    uint16_t yr = t.year;
    date_str[6] = '0' + (char)((yr / 1000) % 10);
    date_str[7] = '0' + (char)((yr / 100) % 10);
    date_str[8] = '0' + (char)((yr / 10) % 10);
    date_str[9] = '0' + (char)(yr % 10);
    date_str[10] = '\0';

    draw_string_px(screen_w - 168, ty + 8, date_str, COLOR_LIGHT_GREY, RGB(25, 25, 40));
}

#define START_MENU_W   160
#define START_MENU_H   120
#define START_ITEM_H   28

static void draw_start_menu(void)
{
    if (!start_menu_visible) return;

    uint32_t mx = 4;
    uint32_t my = screen_h - TASKBAR_HEIGHT - START_MENU_H;

    /* Arka plan */
    fb_fill_rect(mx, my, START_MENU_W, START_MENU_H, RGB(35, 35, 55));
    fb_fill_rect(mx, my, START_MENU_W, 1, RGB(60, 60, 100));
    fb_fill_rect(mx, my, 1, START_MENU_H, RGB(60, 60, 100));
    fb_fill_rect(mx + START_MENU_W - 1, my, 1, START_MENU_H, RGB(60, 60, 100));

    /* Baslik */
    fb_fill_rect(mx + 1, my + 1, START_MENU_W - 2, 24, RGB(40, 80, 170));
    draw_string_px(mx + 8, my + 5, "WintakOS v0.6.0", COLOR_WHITE, RGB(40, 80, 170));

    /* Menu ogeleri */
    draw_string_px(mx + 12, my + 32, "Sistem Bilgisi", COLOR_WHITE, RGB(35, 35, 55));
    fb_fill_rect(mx + 1, my + 52, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 60, "Hakkinda", COLOR_WHITE, RGB(35, 35, 55));
    fb_fill_rect(mx + 1, my + 80, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 88, "Yeniden Baslat", COLOR_LIGHT_GREY, RGB(35, 35, 55));
}

static void cursor_save_bg(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x;
            int32_t py = cy + (int32_t)y;
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h)
                cursor_save_buf[y * CURSOR_W + x] = fb_get_pixel((uint32_t)px, (uint32_t)py);
        }
    }
}

static void cursor_restore_bg(int32_t cx, int32_t cy)
{
    if (cx < 0 && cy < 0) return;
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x;
            int32_t py = cy + (int32_t)y;
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h)
                fb_put_pixel((uint32_t)px, (uint32_t)py, cursor_save_buf[y * CURSOR_W + x]);
        }
    }
}

static void cursor_draw(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x;
            int32_t py = cy + (int32_t)y;
            if (px < 0 || py < 0 || px >= (int32_t)screen_w || py >= (int32_t)screen_h)
                continue;
            uint8_t val = cursor_bitmap[y][x];
            if (val == 1) fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
            else if (val == 2) fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
        }
    }
}

void desktop_init(void)
{
    framebuffer_t* info = fb_get_info();
    screen_w = info->width;
    screen_h = info->height;

    draw_background();
    draw_taskbar();
    wm_init();
    last_cx = -1;
    last_cy = -1;
    last_sec_rtc = 0xFF;
    prev_buttons = 0;
    start_menu_visible = false;
}

bool desktop_start_menu_open(void)
{
    return start_menu_visible;
}

void desktop_update(void)
{
    mouse_state_t ms = mouse_get_state();

    cursor_restore_bg(last_cx, last_cy);

    bool left_pressed = (ms.buttons & MOUSE_BTN_LEFT) && !(prev_buttons & MOUSE_BTN_LEFT);

    /* Start menu tiklama */
    if (left_pressed) {
        uint32_t ty = screen_h - TASKBAR_HEIGHT;

        /* Start butonu alani */
        if (ms.x >= 4 && ms.x < 84 && ms.y >= (int32_t)ty + 4 && ms.y < (int32_t)ty + 28) {
            start_menu_visible = !start_menu_visible;
            wm_set_dirty();
        }
        /* Start menu icerisindeki tiklamalar */
        else if (start_menu_visible) {
            uint32_t mx_menu = 4;
            uint32_t my_menu = screen_h - TASKBAR_HEIGHT - START_MENU_H;

            if (ms.x >= (int32_t)mx_menu && ms.x < (int32_t)(mx_menu + START_MENU_W) &&
                ms.y >= (int32_t)my_menu && ms.y < (int32_t)(my_menu + START_MENU_H)) {
                /* Menu item'a tiklandi — menu'yu kapat */
                start_menu_visible = false;
                wm_set_dirty();
            } else {
                /* Menu disina tiklandi */
                start_menu_visible = false;
                wm_set_dirty();
            }
        }
    }

    wm_handle_mouse(ms.x, ms.y, ms.buttons);
    prev_buttons = ms.buttons;

    /* Full redraw */
    if (wm_is_dirty()) {
        draw_background();
        wm_draw_all();
        draw_taskbar();
        if (start_menu_visible) draw_start_menu();
        wm_clear_dirty();
    } else {
        /* Sadece saat guncelle */
        rtc_time_t t;
        rtc_read(&t);
        if (t.second != last_sec_rtc) {
            last_sec_rtc = t.second;
            draw_taskbar();
            if (start_menu_visible) draw_start_menu();
        }
    }

    cursor_save_bg(ms.x, ms.y);
    cursor_draw(ms.x, ms.y);
    last_cx = ms.x;
    last_cy = ms.y;
}
