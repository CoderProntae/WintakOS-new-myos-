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
static uint32_t last_update_tick = 0;
static bool     busy_cursor = false;
static uint8_t  busy_frame = 0;

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
    draw_string_px(screen_w / 2 - 60, 30, "WintakOS v0.7.0",
                   RGB(200, 200, 255), RGB(22, 32, 84));
}

static void draw_taskbar(void)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;
    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, RGB(25, 25, 40));
    fb_fill_rect(0, ty, screen_w, 1, RGB(60, 60, 100));

    uint32_t btn_c = start_menu_visible ? RGB(70, 110, 200) : RGB(50, 90, 170);
    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, btn_c);
    draw_string_px(12, ty + 8, "WintakOS", COLOR_WHITE, btn_c);

    rtc_time_t t;
    rtc_read(&t);
    char ts[9];
    ts[0]='0'+(char)(t.hour/10); ts[1]='0'+(char)(t.hour%10); ts[2]=':';
    ts[3]='0'+(char)(t.minute/10); ts[4]='0'+(char)(t.minute%10); ts[5]=':';
    ts[6]='0'+(char)(t.second/10); ts[7]='0'+(char)(t.second%10); ts[8]=0;
    draw_string_px(screen_w - 80, ty + 8, ts, COLOR_WHITE, RGB(25, 25, 40));

    char ds[11];
    ds[0]='0'+(char)(t.day/10); ds[1]='0'+(char)(t.day%10); ds[2]='/';
    ds[3]='0'+(char)(t.month/10); ds[4]='0'+(char)(t.month%10); ds[5]='/';
    ds[6]='0'+(char)((t.year/1000)%10); ds[7]='0'+(char)((t.year/100)%10);
    ds[8]='0'+(char)((t.year/10)%10); ds[9]='0'+(char)(t.year%10); ds[10]=0;
    draw_string_px(screen_w - 168, ty + 8, ds, COLOR_LIGHT_GREY, RGB(25, 25, 40));
}

#define START_MENU_W 160
#define START_MENU_H 120

static void draw_start_menu(void)
{
    if (!start_menu_visible) return;
    uint32_t mx = 4;
    uint32_t my = screen_h - TASKBAR_HEIGHT - START_MENU_H;

    fb_fill_rect(mx, my, START_MENU_W, START_MENU_H, RGB(35, 35, 55));
    fb_fill_rect(mx, my, START_MENU_W, 1, RGB(60, 60, 100));
    fb_fill_rect(mx, my, 1, START_MENU_H, RGB(60, 60, 100));
    fb_fill_rect(mx + START_MENU_W - 1, my, 1, START_MENU_H, RGB(60, 60, 100));

    fb_fill_rect(mx + 1, my + 1, START_MENU_W - 2, 24, RGB(40, 80, 170));
    draw_string_px(mx + 8, my + 5, "WintakOS v0.7.0", COLOR_WHITE, RGB(40, 80, 170));

    draw_string_px(mx + 12, my + 32, "Terminal", COLOR_WHITE, RGB(35, 35, 55));
    fb_fill_rect(mx + 1, my + 52, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 60, "Sistem Bilgisi", COLOR_WHITE, RGB(35, 35, 55));
    fb_fill_rect(mx + 1, my + 80, START_MENU_W - 2, 1, RGB(60, 60, 80));
    draw_string_px(mx + 12, my + 88, "Hakkinda", COLOR_LIGHT_GREY, RGB(35, 35, 55));
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

    /* Busy indicator — ok imlecinin sag altinda */
    if (busy_cursor) {
        int32_t bx = cx + 14;
        int32_t by = cy + 14;
        for (uint32_t y = 0; y < BUSY_SIZE; y++)
            for (uint32_t x = 0; x < BUSY_SIZE; x++) {
                int32_t px = bx + (int32_t)x, py = by + (int32_t)y;
                if (px < 0 || py < 0 || px >= (int32_t)screen_w || py >= (int32_t)screen_h) continue;
                uint8_t v = busy_bitmap[busy_frame][y][x];
                if (v == 1) fb_put_pixel((uint32_t)px, (uint32_t)py, RGB(100, 100, 140));
                else if (v == 2) fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
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
    last_cx = -1; last_cy = -1;
    last_sec_rtc = 0xFF;
    prev_buttons = 0;
    start_menu_visible = false;
    last_update_tick = 0;
    busy_cursor = false;
    busy_frame = 0;
}

void desktop_set_busy(bool b)
{
    busy_cursor = b;
    if (!b) busy_frame = 0;
}

bool desktop_start_menu_open(void)
{
    return start_menu_visible;
}

/* start menu'de hangi item'a tiklandi: 0=terminal, 1=sysinfo, 2=hakkinda, -1=yok */
int desktop_start_menu_click(int32_t mx, int32_t my)
{
    uint32_t smx = 4;
    uint32_t smy = screen_h - TASKBAR_HEIGHT - START_MENU_H;

    if (mx < (int32_t)smx || mx >= (int32_t)(smx + START_MENU_W)) return -1;
    if (my < (int32_t)smy || my >= (int32_t)(smy + START_MENU_H)) return -1;

    int32_t ry = my - (int32_t)smy;
    if (ry >= 28 && ry < 52) return 0;  /* Terminal */
    if (ry >= 56 && ry < 80) return 1;  /* Sistem Bilgisi */
    if (ry >= 84 && ry < 108) return 2; /* Hakkinda */
    return -1;
}

void desktop_update(void)
{
    /* Frame limiter: ~30 FPS */
    uint32_t now = pit_get_ticks();
    if (now - last_update_tick < 3) return;
    last_update_tick = now;

    mouse_state_t ms = mouse_get_state();
    bool mouse_moved = (ms.x != last_cx || ms.y != last_cy);
    bool left_pressed = (ms.buttons & MOUSE_BTN_LEFT) && !(prev_buttons & MOUSE_BTN_LEFT);

    /* Busy cursor animasyonu */
    if (busy_cursor && (now % 8 == 0)) {
        busy_frame = (busy_frame + 1) % BUSY_FRAMES;
        mouse_moved = true; /* redraw cursor */
    }

    /* Start menu tiklama */
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

    /* Full redraw sadece dirty ise */
    if (wm_is_dirty()) {
        cursor_restore_bg(last_cx, last_cy);
        draw_background();
        wm_draw_all();
        draw_taskbar();
        if (start_menu_visible) draw_start_menu();
        wm_clear_dirty();
        cursor_save_bg(ms.x, ms.y);
        cursor_draw(ms.x, ms.y);
        last_cx = ms.x; last_cy = ms.y;
    }
    else if (mouse_moved) {
        /* Sadece cursor guncelle */
        cursor_restore_bg(last_cx, last_cy);
        cursor_save_bg(ms.x, ms.y);
        cursor_draw(ms.x, ms.y);
        last_cx = ms.x; last_cy = ms.y;
    }
    else {
        /* Sadece saat */
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
