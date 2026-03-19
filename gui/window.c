#include "window.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../lib/string.h"

static window_t windows[MAX_WINDOWS];
static uint8_t  win_order[MAX_WINDOWS];
static uint32_t win_count = 0;

static int32_t  drag_offset_x = 0;
static int32_t  drag_offset_y = 0;
static window_t* dragging_win = NULL;
static bool     global_dirty = true;
static uint8_t  prev_btn = 0;

/* Pencere sinirlarini kontrol et */
static void clamp_window(window_t* win)
{
    framebuffer_t* fb = fb_get_info();
    int32_t sw = (int32_t)fb->width;
    int32_t sh = (int32_t)fb->height - 32; /* taskbar */

    /* En az 60px ekranda gorunsun */
    if (win->x < -(int32_t)win->width + 60)
        win->x = -(int32_t)win->width + 60;
    if (win->x > sw - 60)
        win->x = sw - 60;

    /* Ust sinir: titlebar gorunsun */
    if (win->y < (int32_t)TITLEBAR_HEIGHT + 2)
        win->y = (int32_t)TITLEBAR_HEIGHT + 2;

    /* Alt sinir: taskbar ustunde kalsin */
    if (win->y > sh - 20)
        win->y = sh - 20;
}

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    if (px >= fb->width - 8 || py >= fb->height - 16) return;
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
    framebuffer_t* fb = fb_get_info();
    while (*str) {
        if (px >= fb->width - 8) break;
        draw_char_at(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static void safe_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color)
{
    framebuffer_t* fb = fb_get_info();
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (w <= 0 || h <= 0) return;
    if (x >= (int32_t)fb->width || y >= (int32_t)fb->height) return;
    if (x + w > (int32_t)fb->width) w = (int32_t)fb->width - x;
    if (y + h > (int32_t)fb->height) h = (int32_t)fb->height - y;
    fb_fill_rect((uint32_t)x, (uint32_t)y, (uint32_t)w, (uint32_t)h, color);
}

static void draw_window(window_t* win)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;

    int32_t wx = win->x;
    int32_t wy = win->y;
    framebuffer_t* fb = fb_get_info();

    if (wx + (int32_t)win->width < 0 || wy + (int32_t)win->height < 0) return;
    if (wx > (int32_t)fb->width || wy > (int32_t)fb->height) return;

    uint32_t title_bg = win->active ? RGB(40, 80, 170) : RGB(80, 80, 100);
    uint32_t border_c = win->active ? RGB(60, 120, 210) : RGB(100, 100, 120);

    /* Border */
    safe_fill_rect(wx - (int32_t)BORDER_WIDTH,
                   wy - (int32_t)TITLEBAR_HEIGHT - (int32_t)BORDER_WIDTH,
                   (int32_t)win->width + (int32_t)BORDER_WIDTH * 2,
                   (int32_t)win->height + (int32_t)TITLEBAR_HEIGHT + (int32_t)BORDER_WIDTH * 2,
                   border_c);

    /* Titlebar */
    safe_fill_rect(wx, wy - (int32_t)TITLEBAR_HEIGHT,
                   (int32_t)win->width, (int32_t)TITLEBAR_HEIGHT, title_bg);

    if (wx >= 0 && wy >= (int32_t)TITLEBAR_HEIGHT) {
        draw_string_at((uint32_t)wx + 8,
                       (uint32_t)(wy - (int32_t)TITLEBAR_HEIGHT) + 4,
                       win->title, COLOR_WHITE, title_bg);
    }

    /* Close */
    if (win->flags & WIN_FLAG_CLOSABLE) {
        int32_t cbx = wx + (int32_t)win->width - CLOSE_BTN_SIZE - 4;
        int32_t cby = wy - (int32_t)TITLEBAR_HEIGHT + 4;
        if (cbx > 0 && cby > 0 && cbx < (int32_t)fb->width - 16 && cby < (int32_t)fb->height - 16) {
            fb_fill_rect((uint32_t)cbx, (uint32_t)cby,
                         CLOSE_BTN_SIZE, CLOSE_BTN_SIZE, RGB(200, 50, 50));
            draw_char_at((uint32_t)cbx + 4, (uint32_t)cby, 'X',
                         COLOR_WHITE, RGB(200, 50, 50));
        }
    }

    /* Client area */
    safe_fill_rect(wx, wy, (int32_t)win->width, (int32_t)win->height, win->bg_color);

    if (win->on_draw) win->on_draw(win);
}

void wm_init(void)
{
    memset(windows, 0, sizeof(windows));
    memset(win_order, 0, sizeof(win_order));
    win_count = 0;
    dragging_win = NULL;
    global_dirty = true;
    prev_btn = 0;
}

window_t* wm_create_window(int32_t x, int32_t y, uint32_t w, uint32_t h,
                            const char* title, uint32_t bg_color)
{
    if (win_count >= MAX_WINDOWS) return NULL;

    window_t* win = &windows[win_count];
    win->x = x;
    win->y = y + (int32_t)TITLEBAR_HEIGHT;
    win->width = w;
    win->height = h;
    win->bg_color = bg_color;
    win->flags = WIN_FLAG_VISIBLE | WIN_FLAG_CLOSABLE;
    win->id = (uint8_t)win_count;
    win->active = false;
    win->on_draw = NULL;
    win->on_click = NULL;

    uint32_t i;
    for (i = 0; i < 63 && title[i]; i++)
        win->title[i] = title[i];
    win->title[i] = '\0';

    for (uint32_t j = 0; j < win_count; j++)
        windows[j].active = false;
    win->active = true;

    clamp_window(win);

    win_order[win_count] = (uint8_t)win_count;
    win_count++;
    global_dirty = true;
    return win;
}

void wm_close_window(window_t* win)
{
    if (!win) return;
    win->flags &= ~WIN_FLAG_VISIBLE;
    global_dirty = true;
}

void wm_draw_all(void)
{
    for (uint32_t i = 0; i < win_count; i++)
        draw_window(&windows[win_order[i]]);
}

static void bring_to_front(uint32_t order_idx)
{
    if (order_idx >= win_count) return;
    uint8_t target = win_order[order_idx];
    for (uint32_t i = order_idx; i < win_count - 1; i++)
        win_order[i] = win_order[i + 1];
    win_order[win_count - 1] = target;
    for (uint32_t i = 0; i < win_count; i++)
        windows[i].active = false;
    windows[target].active = true;
    global_dirty = true;
}

void wm_handle_mouse(int32_t mx, int32_t my, uint8_t buttons)
{
    bool left_down = (buttons & MOUSE_BTN_LEFT) != 0;
    bool left_pressed = left_down && !(prev_btn & MOUSE_BTN_LEFT);
    prev_btn = buttons;

    if (dragging_win && left_down) {
        int32_t nx = mx - drag_offset_x;
        int32_t ny = my - drag_offset_y;
        dragging_win->x = nx;
        dragging_win->y = ny;
        clamp_window(dragging_win);
        global_dirty = true;
        return;
    }

    if (!left_down) { dragging_win = NULL; return; }
    if (!left_pressed) return;

    for (int32_t i = (int32_t)win_count - 1; i >= 0; i--) {
        window_t* win = &windows[win_order[i]];
        if (!(win->flags & WIN_FLAG_VISIBLE)) continue;

        int32_t wx = win->x;
        int32_t wy = win->y - (int32_t)TITLEBAR_HEIGHT;
        int32_t wr = wx + (int32_t)win->width;
        int32_t wb = win->y + (int32_t)win->height;

        if (mx >= wx && mx < wr && my >= wy && my < wb) {
            bring_to_front((uint32_t)i);

            if (win->flags & WIN_FLAG_CLOSABLE) {
                int32_t cbx = wx + (int32_t)win->width - CLOSE_BTN_SIZE - 4;
                int32_t cby = wy + 4;
                if (mx >= cbx && mx < cbx + (int32_t)CLOSE_BTN_SIZE &&
                    my >= cby && my < cby + (int32_t)CLOSE_BTN_SIZE) {
                    wm_close_window(win);
                    dragging_win = NULL;
                    return;
                }
            }

            if (my >= win->y && win->on_click) {
                win->on_click(win, mx - win->x, my - win->y);
            }

            if (my >= wy && my < win->y) {
                dragging_win = win;
                drag_offset_x = mx - win->x;
                drag_offset_y = my - win->y;
            }
            return;
        }
    }
}

window_t* wm_get_window(uint32_t index)
{
    if (index >= win_count) return NULL;
    return &windows[index];
}

uint32_t wm_get_count(void) { return win_count; }
bool wm_is_dirty(void) { return global_dirty; }
void wm_clear_dirty(void) { global_dirty = false; }
void wm_set_dirty(void) { global_dirty = true; }
