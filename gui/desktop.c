#include "desktop.h"
#include "window.h"
#include "cursor.h"
#include "../drivers/framebuffer.h"
#include "../drivers/fbconsole.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"

static uint32_t screen_w = 800;
static uint32_t screen_h = 600;
static uint32_t cursor_save[CURSOR_W * CURSOR_H];
static int32_t  last_cx = -1, last_cy = -1;

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

static void cursor_save_bg(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            cursor_save[y * CURSOR_W + x] = fb_get_pixel((uint32_t)(cx + (int32_t)x),
                                                          (uint32_t)(cy + (int32_t)y));
        }
    }
}

static void cursor_restore_bg(int32_t cx, int32_t cy)
{
    if (cx < 0 || cy < 0) return;
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            fb_put_pixel((uint32_t)(cx + (int32_t)x),
                         (uint32_t)(cy + (int32_t)y),
                         cursor_save[y * CURSOR_W + x]);
        }
    }
}

static void cursor_draw(int32_t cx, int32_t cy)
{
    for (uint32_t y = 0; y < CURSOR_H; y++) {
        for (uint32_t x = 0; x < CURSOR_W; x++) {
            uint8_t val = cursor_bitmap[y][x];
            if (val == 1)
                fb_put_pixel((uint32_t)(cx + (int32_t)x), (uint32_t)(cy + (int32_t)y), COLOR_BLACK);
            else if (val == 2)
                fb_put_pixel((uint32_t)(cx + (int32_t)x), (uint32_t)(cy + (int32_t)y), COLOR_WHITE);
        }
    }
}

void desktop_init(void)
{
    framebuffer_t* info = fb_get_info();
    screen_w = info->width;
    screen_h = info->height;

    /* Masaustu arka plan — gradient */
    for (uint32_t y = 0; y < screen_h - TASKBAR_HEIGHT; y++) {
        uint8_t r = (uint8_t)(20 + y * 30 / screen_h);
        uint8_t g = (uint8_t)(30 + y * 40 / screen_h);
        uint8_t b = (uint8_t)(80 + y * 60 / screen_h);
        uint32_t color = RGB(r, g, b);
        for (uint32_t x = 0; x < screen_w; x++) {
            fb_put_pixel(x, y, color);
        }
    }

    /* Masaustu yazisi */
    draw_string_px(screen_w / 2 - 60, screen_h / 2 - 40, "WintakOS v0.6.0",
                   RGB(255, 255, 255), RGB(30, 40, 90));

    wm_init();
    last_cx = -1;
    last_cy = -1;
}

void desktop_draw_taskbar(uint32_t ticks, uint32_t frequency)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;

    /* Taskbar arka plan */
    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, RGB(25, 25, 40));

    /* Ust cizgi */
    fb_fill_rect(0, ty, screen_w, 1, RGB(60, 60, 100));

    /* WintakOS butonu */
    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, RGB(50, 90, 170));
    draw_string_px(12, ty + 8, "WintakOS", COLOR_WHITE, RGB(50, 90, 170));

    /* Saat */
    uint32_t total_sec = ticks / frequency;
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

void desktop_draw(void)
{
    desktop_draw_taskbar(pit_get_ticks(), PIT_FREQUENCY);
    wm_draw_all();
}

void desktop_update(void)
{
    mouse_state_t ms = mouse_get_state();

    /* Eski imleci geri yukle */
    cursor_restore_bg(last_cx, last_cy);

    /* Mouse olaylarini isle */
    wm_handle_mouse(ms.x, ms.y, ms.buttons);

    /* Taskbar guncelle */
    desktop_draw_taskbar(pit_get_ticks(), PIT_FREQUENCY);

    /* Pencereleri ciz */
    wm_draw_all();

    /* Yeni imleç konumunu kaydet ve ciz */
    cursor_save_bg(ms.x, ms.y);
    cursor_draw(ms.x, ms.y);
    last_cx = ms.x;
    last_cy = ms.y;

    mouse_clear_moved();
}
