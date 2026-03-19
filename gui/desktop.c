#include "desktop.h"
#include "window.h"
#include "cursor.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

static uint32_t screen_w = 800;
static uint32_t screen_h = 600;

static uint32_t cursor_save_buf[CURSOR_W * CURSOR_H];
static int32_t  last_cx = -1;
static int32_t  last_cy = -1;

static uint32_t last_second = 0xFFFFFFFF;
static uint8_t  prev_buttons = 0;

static content_draw_fn draw_content = NULL;

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

    /* Center logo */
    draw_string_px(screen_w / 2 - 60, 30, "WintakOS v0.6.0",
                   RGB(200, 200, 255), RGB(22, 32, 84));
}

static void draw_taskbar(void)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;

    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, RGB(25, 25, 40));
    fb_fill_rect(0, ty, screen_w, 1, RGB(60, 60, 100));

    /* Start button */
    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, RGB(50, 90, 170));
    draw_string_px(12, ty + 8, "WintakOS", COLOR_WHITE, RGB(50, 90, 170));

    /* Uptime clock */
    uint32_t total_sec = pit_get_ticks() / PIT_FREQUENCY;
    uint32_t hours   = total_sec / 3600;
    uint32_t minutes = (total_sec % 3600) / 60;
    uint32_t seconds = total_sec % 60;

    char time_str[9];
    time_str[0] = '0' + (char)(hours / 10);
    time_str[1] = '0' + (char)(hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (char)(minutes / 10);
    time_str[4] = '0' + (char)(minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (char)(seconds / 10);
    time_str[7] = '0' + (char)(seconds % 10);
    time_str[8] = '\0';

    draw_string_px(screen_w - 80, ty + 8, time_str, COLOR_WHITE, RGB(25, 25, 40));
}

static void cursor_save_bg(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            int32_t px = cx + (int32_t)x;
            int32_t py = cy + (int32_t)y;
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h) {
                cursor_save_buf[y * CURSOR_W + x] = fb_get_pixel((uint32_t)px, (uint32_t)py);
            }
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
            if (px >= 0 && py >= 0 && px < (int32_t)screen_w && py < (int32_t)screen_h) {
                fb_put_pixel((uint32_t)px, (uint32_t)py, cursor_save_buf[y * CURSOR_W + x]);
            }
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
            if (val == 1)
                fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
            else if (val == 2)
                fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
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
    last_second = 0xFFFFFFFF;
    prev_buttons = 0;
}

void desktop_set_content_drawer(content_draw_fn fn)
{
    draw_content = fn;
}

void desktop_update(void)
{
    mouse_state_t ms = mouse_get_state();

    /* 1. Restore old cursor */
    cursor_restore_bg(last_cx, last_cy);

    /* 2. Handle mouse input */
    bool clicked = (ms.buttons & MOUSE_BTN_LEFT) && !(prev_buttons & MOUSE_BTN_LEFT);
    (void)clicked;

    wm_handle_mouse(ms.x, ms.y, ms.buttons);
    prev_buttons = ms.buttons;

    /* 3. Full redraw if something changed */
    if (wm_is_dirty()) {
        draw_background();
        wm_draw_all();

        if (draw_content) {
            draw_content();
        }

        draw_taskbar();
        wm_clear_dirty();
    } else {
        /* 4. Only update taskbar clock every second */
        uint32_t now = pit_get_ticks() / PIT_FREQUENCY;
        if (now != last_second) {
            last_second = now;
            draw_taskbar();
        }
    }

    /* 5. Save + draw cursor */
    cursor_save_bg(ms.x, ms.y);
    cursor_draw(ms.x, ms.y);
    last_cx = ms.x;
    last_cy = ms.y;
}
