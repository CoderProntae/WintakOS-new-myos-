#include "window.h"
#include "../drivers/framebuffer.h"
#include "../drivers/fbconsole.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../lib/string.h"

static window_t windows[MAX_WINDOWS];
static uint8_t  win_order[MAX_WINDOWS];
static uint32_t win_count = 0;
static int32_t  drag_offset_x = 0;
static int32_t  drag_offset_y = 0;
static window_t* dragging_win = NULL;

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    const uint8_t* glyph = font8x16_data[c < 128 ? c : 0];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++) {
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
        }
    }
}

static void draw_string_at(uint32_t px, uint32_t py, const char* str, uint32_t fg, uint32_t bg)
{
    while (*str) {
        draw_char_at(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static void draw_window(window_t* win)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;

    uint32_t title_bg = win->active ? RGB(40, 80, 160) : RGB(80, 80, 100);
    uint32_t border_c = win->active ? RGB(60, 120, 200) : RGB(100, 100, 120);

    /* Border */
    fb_fill_rect((uint32_t)win->x - BORDER_WIDTH, (uint32_t)win->y - TITLEBAR_HEIGHT - BORDER_WIDTH,
                 win->width + BORDER_WIDTH * 2, win->height + TITLEBAR_HEIGHT + BORDER_WIDTH * 2,
                 border_c);

    /* Titlebar */
    fb_fill_rect((uint32_t)win->x, (uint32_t)win->y - TITLEBAR_HEIGHT,
                 win->width, TITLEBAR_HEIGHT, title_bg);

    /* Title text */
    draw_string_at((uint32_t)win->x + 8, (uint32_t)win->y - TITLEBAR_HEIGHT + 4,
                   win->title, COLOR_WHITE, title_bg);

    /* Close button */
    if (win->flags & WIN_FLAG_CLOSABLE) {
        uint32_t bx = (uint32_t)win->x + win->width - CLOSE_BTN_SIZE - 4;
        uint32_t by = (uint32_t)win->y - TITLEBAR_HEIGHT + 4;
        fb_fill_rect(bx, by, CLOSE_BTN_SIZE, CLOSE_BTN_SIZE, RGB(200, 50, 50));
        draw_char_at(bx + 4, by, 'X', COLOR_WHITE, RGB(200, 50, 50));
    }

    /* Client area */
    fb_fill_rect((uint32_t)win->x, (uint32_t)win->y, win->width, win->height, win->bg_color);

    win->dirty = false;
}

void wm_init(void)
{
    memset(windows, 0, sizeof(windows));
    memset(win_order, 0, sizeof(win_order));
    win_count = 0;
    dragging_win = NULL;
}

window_t* wm_create_window(int32_t x, int32_t y, uint32_t w, uint32_t h,
                            const char* title, uint32_t bg_color)
{
    if (win_count >= MAX_WINDOWS) return NULL;

    window_t* win = &windows[win_count];
    win->x = x;
    win->y = y + TITLEBAR_HEIGHT;
    win->width = w;
    win->height = h;
    win->bg_color = bg_color;
    win->title_color = COLOR_WHITE;
    win->flags = WIN_FLAG_VISIBLE | WIN_FLAG_CLOSABLE;
    win->id = (uint8_t)win_count;
    win->active = true;
    win->dirty = true;

    /* Title kopyala */
    uint32_t i;
    for (i = 0; i < 63 && title[i]; i++) {
        win->title[i] = title[i];
    }
    win->title[i] = '\0';

    /* Onceki aktif pencereyi deaktif yap */
    for (uint32_t j = 0; j < win_count; j++) {
        windows[j].active = false;
    }

    win_order[win_count] = (uint8_t)win_count;
    win_count++;

    return win;
}

void wm_close_window(window_t* win)
{
    if (!win) return;
    win->flags &= ~WIN_FLAG_VISIBLE;
}

void wm_draw_all(void)
{
    for (uint32_t i = 0; i < win_count; i++) {
        draw_window(&windows[win_order[i]]);
    }
}

static void bring_to_front(uint32_t idx)
{
    uint8_t target = win_order[idx];
    for (uint32_t i = idx; i < win_count - 1; i++) {
        win_order[i] = win_order[i + 1];
    }
    win_order[win_count - 1] = target;

    for (uint32_t i = 0; i < win_count; i++) {
        windows[i].active = false;
    }
    windows[target].active = true;
}

void wm_handle_mouse(int32_t mx, int32_t my, uint8_t buttons)
{
    /* Surukleme devam ediyor mu? */
    if (dragging_win && (buttons & MOUSE_BTN_LEFT)) {
        dragging_win->x = mx - drag_offset_x;
        dragging_win->y = my - drag_offset_y;
        return;
    }
    dragging_win = NULL;

    if (!(buttons & MOUSE_BTN_LEFT)) return;

    /* Ust pencereden baslayarak tiklanani bul */
    for (int32_t i = (int32_t)win_count - 1; i >= 0; i--) {
        window_t* win = &windows[win_order[i]];
        if (!(win->flags & WIN_FLAG_VISIBLE)) continue;

        int32_t wx = win->x;
        int32_t wy = win->y - TITLEBAR_HEIGHT;
        int32_t wr = wx + (int32_t)win->width;
        int32_t wb = win->y + (int32_t)win->height;

        if (mx >= wx && mx < wr && my >= wy && my < wb) {
            bring_to_front((uint32_t)i);

            /* Close butonu? */
            if (win->flags & WIN_FLAG_CLOSABLE) {
                int32_t cbx = wx + (int32_t)win->width - CLOSE_BTN_SIZE - 4;
                int32_t cby = wy + 4;
                if (mx >= cbx && mx < cbx + (int32_t)CLOSE_BTN_SIZE &&
                    my >= cby && my < cby + (int32_t)CLOSE_BTN_SIZE) {
                    wm_close_window(win);
                    return;
                }
            }

            /* Titlebar surukle? */
            if (my >= wy && my < win->y) {
                dragging_win = win;
                drag_offset_x = mx - win->x;
                drag_offset_y = my - win->y;
            }
            return;
        }
    }
}

window_t* wm_get_active(void)
{
    for (uint32_t i = 0; i < win_count; i++) {
        if (windows[i].active && (windows[i].flags & WIN_FLAG_VISIBLE))
            return &windows[i];
    }
    return NULL;
}

uint32_t wm_get_count(void)
{
    return win_count;
}
