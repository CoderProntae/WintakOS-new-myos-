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
static bool     scrollbar_dragging = false;
static int32_t  scroll_drag_offset = 0;
static window_t* scroll_drag_win = NULL;
static bool     global_dirty = true;
static uint8_t  prev_btn = 0;

#define SCROLLBAR_WIDTH 14

/* ---- Yardimci cizim ---- */

static void clamp_window(window_t* win)
{
    framebuffer_t* fb = fb_get_info();
    int32_t sw = (int32_t)fb->width;
    int32_t sh = (int32_t)fb->height - 32;

    if (win->x < -(int32_t)win->width + 60)
        win->x = -(int32_t)win->width + 60;
    if (win->x > sw - 60)
        win->x = sw - 60;
    if (win->y < (int32_t)TITLEBAR_HEIGHT + 2)
        win->y = (int32_t)TITLEBAR_HEIGHT + 2;
    if (win->y > sh - 20)
        win->y = sh - 20;
}

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c,
                          uint32_t fg, uint32_t bg)
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

static void draw_string_at(uint32_t px, uint32_t py, const char* str,
                            uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    while (*str) {
        if (px >= fb->width - 8) break;
        draw_char_at(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static void safe_fill_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                            uint32_t color)
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

/* ---- Scrollbar cizimi ---- */

static void draw_scrollbar(window_t* win)
{
    if (!(win->flags & WIN_FLAG_HAS_SCROLL)) return;
    if (win->scroll_total <= win->scroll_visible) return;

    int32_t sx = win->x + (int32_t)win->width - (int32_t)SCROLLBAR_WIDTH;
    int32_t sy = win->y;
    uint32_t sh = win->height;

    /* Scrollbar arka plan */
    safe_fill_rect(sx, sy, SCROLLBAR_WIDTH, (int32_t)sh, RGB(50, 50, 70));

    /* Thumb hesapla */
    uint32_t thumb_h = (win->scroll_visible * sh) / win->scroll_total;
    if (thumb_h < 20) thumb_h = 20;
    if (thumb_h > sh) thumb_h = sh;

    uint32_t max_scroll = win->scroll_total - win->scroll_visible;
    uint32_t thumb_y_offset = 0;
    if (max_scroll > 0)
        thumb_y_offset = (win->scroll_pos * (sh - thumb_h)) / max_scroll;

    uint32_t thumb_color = win->active ? RGB(100, 140, 220) : RGB(120, 120, 140);
    safe_fill_rect(sx + 2, sy + (int32_t)thumb_y_offset,
                   SCROLLBAR_WIDTH - 4, (int32_t)thumb_h, thumb_color);
}

/* ---- Pencere cizimi ---- */

static void draw_title_button(int32_t bx, int32_t by, uint32_t color,
                               const char* symbol)
{
    framebuffer_t* fb = fb_get_info();
    if (bx < 0 || by < 0 || bx >= (int32_t)fb->width - 16 ||
        by >= (int32_t)fb->height - 16) return;

    fb_fill_rect((uint32_t)bx, (uint32_t)by,
                 TITLE_BTN_SIZE, TITLE_BTN_SIZE, color);
    draw_char_at((uint32_t)bx + 4, (uint32_t)by, (uint8_t)symbol[0],
                 COLOR_WHITE, color);
}

static void draw_window(window_t* win)
{
    if (!(win->flags & WIN_FLAG_VISIBLE)) return;
    if (win->flags & WIN_FLAG_MINIMIZED) return;

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
                   (int32_t)win->height + (int32_t)TITLEBAR_HEIGHT +
                   (int32_t)BORDER_WIDTH * 2,
                   border_c);

    /* Titlebar */
    safe_fill_rect(wx, wy - (int32_t)TITLEBAR_HEIGHT,
                   (int32_t)win->width, (int32_t)TITLEBAR_HEIGHT, title_bg);

    /* Baslik metni */
    if (wx >= 0 && wy >= (int32_t)TITLEBAR_HEIGHT) {
        draw_string_at((uint32_t)wx + 8,
                       (uint32_t)(wy - (int32_t)TITLEBAR_HEIGHT) + 4,
                       win->title, COLOR_WHITE, title_bg);
    }

    /* Kapatma butonu (X) */
    if (win->flags & WIN_FLAG_CLOSABLE) {
        int32_t cbx = wx + (int32_t)win->width - TITLE_BTN_SIZE - 4;
        int32_t cby = wy - (int32_t)TITLEBAR_HEIGHT + 4;
        draw_title_button(cbx, cby, RGB(200, 50, 50), "X");
    }

    /* Maximize/Restore butonu */
    {
        int32_t mbx = wx + (int32_t)win->width - (TITLE_BTN_SIZE + TITLE_BTN_GAP) * 2 - 4;
        int32_t mby = wy - (int32_t)TITLEBAR_HEIGHT + 4;
        if (win->flags & WIN_FLAG_MAXIMIZED) {
            draw_title_button(mbx, mby, RGB(60, 130, 60), "r");
        } else {
            draw_title_button(mbx, mby, RGB(60, 130, 60), "M");
        }
    }

    /* Minimize butonu (_) */
    {
        int32_t nbx = wx + (int32_t)win->width - (TITLE_BTN_SIZE + TITLE_BTN_GAP) * 3 - 4;
        int32_t nby = wy - (int32_t)TITLEBAR_HEIGHT + 4;
        draw_title_button(nbx, nby, RGB(180, 160, 40), "_");
    }

    /* Client area */
    safe_fill_rect(wx, wy, (int32_t)win->width, (int32_t)win->height,
                   win->bg_color);

    /* Icerik ciz */
    if (win->on_draw) win->on_draw(win);

    /* Scrollbar */
    draw_scrollbar(win);
}

/* ---- Window Manager ---- */

void wm_init(void)
{
    memset(windows, 0, sizeof(windows));
    memset(win_order, 0, sizeof(win_order));
    win_count = 0;
    dragging_win = NULL;
    scrollbar_dragging = false;
    scroll_drag_win = NULL;
    global_dirty = true;
    prev_btn = 0;
}

window_t* wm_create_window(int32_t x, int32_t y, uint32_t w, uint32_t h,
                            const char* title, uint32_t bg_color)
{
    if (win_count >= MAX_WINDOWS) return NULL;

    window_t* win = &windows[win_count];
    memset(win, 0, sizeof(window_t));
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
    win->on_scroll = NULL;
    win->on_rightclick = NULL;
    win->saved_x = x;
    win->saved_y = y + (int32_t)TITLEBAR_HEIGHT;
    win->saved_w = w;
    win->saved_h = h;
    win->scroll_total = 0;
    win->scroll_visible = 0;
    win->scroll_pos = 0;

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
    win->flags &= (uint8_t)~WIN_FLAG_VISIBLE;
    global_dirty = true;
}

void wm_minimize_window(window_t* win)
{
    if (!win) return;
    win->flags |= WIN_FLAG_MINIMIZED;
    global_dirty = true;
}

void wm_maximize_window(window_t* win)
{
    if (!win) return;
    framebuffer_t* fb = fb_get_info();

    /* Mevcut konumu kaydet */
    win->saved_x = win->x;
    win->saved_y = win->y;
    win->saved_w = win->width;
    win->saved_h = win->height;

    /* Tam ekran yap */
    win->x = 0;
    win->y = (int32_t)TITLEBAR_HEIGHT + (int32_t)BORDER_WIDTH;
    win->width = fb->width;
    win->height = fb->height - TITLEBAR_HEIGHT - BORDER_WIDTH - 32;
    win->flags |= WIN_FLAG_MAXIMIZED;
    global_dirty = true;
}

void wm_restore_window(window_t* win)
{
    if (!win) return;

    if (win->flags & WIN_FLAG_MINIMIZED) {
        win->flags &= (uint8_t)~WIN_FLAG_MINIMIZED;
        global_dirty = true;
        return;
    }

    if (win->flags & WIN_FLAG_MAXIMIZED) {
        win->x = win->saved_x;
        win->y = win->saved_y;
        win->width = win->saved_w;
        win->height = win->saved_h;
        win->flags &= (uint8_t)~WIN_FLAG_MAXIMIZED;
        global_dirty = true;
    }
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

/* Scrollbar tiklama kontrolu */
static bool handle_scrollbar_click(window_t* win, int32_t mx, int32_t my)
{
    if (!(win->flags & WIN_FLAG_HAS_SCROLL)) return false;
    if (win->scroll_total <= win->scroll_visible) return false;

    int32_t sx = win->x + (int32_t)win->width - (int32_t)SCROLLBAR_WIDTH;
    if (mx < sx || mx >= sx + (int32_t)SCROLLBAR_WIDTH) return false;
    if (my < win->y || my >= win->y + (int32_t)win->height) return false;

    /* Thumb pozisyonu hesapla */
    uint32_t sh = win->height;
    uint32_t thumb_h = (win->scroll_visible * sh) / win->scroll_total;
    if (thumb_h < 20) thumb_h = 20;
    uint32_t max_scroll = win->scroll_total - win->scroll_visible;
    uint32_t thumb_y = 0;
    if (max_scroll > 0)
        thumb_y = (win->scroll_pos * (sh - thumb_h)) / max_scroll;

    int32_t thumb_screen_y = win->y + (int32_t)thumb_y;

    if (my >= thumb_screen_y && my < thumb_screen_y + (int32_t)thumb_h) {
        scrollbar_dragging = true;
        scroll_drag_win = win;
        scroll_drag_offset = my - thumb_screen_y;
    } else {
        /* Thumb disina tiklanirsa o noktaya atla */
        int32_t rel = my - win->y;
        if (rel >= 0 && sh > thumb_h) {
            uint32_t new_pos = ((uint32_t)rel * max_scroll) / (sh - thumb_h);
            if (new_pos > max_scroll) new_pos = max_scroll;
            win->scroll_pos = new_pos;
            if (win->on_scroll) win->on_scroll(win, (int32_t)win->scroll_pos);
            global_dirty = true;
        }
    }
    return true;
}

void wm_handle_mouse(int32_t mx, int32_t my, uint8_t buttons)
{
    bool left_down = (buttons & MOUSE_BTN_LEFT) != 0;
    bool left_pressed = left_down && !(prev_btn & MOUSE_BTN_LEFT);
    bool right_pressed = (buttons & MOUSE_BTN_RIGHT) &&
                         !(prev_btn & MOUSE_BTN_RIGHT);
    prev_btn = buttons;

    /* Scrollbar surukle */
    if (scrollbar_dragging && left_down && scroll_drag_win) {
        window_t* win = scroll_drag_win;
        uint32_t sh = win->height;
        uint32_t thumb_h = (win->scroll_visible * sh) / win->scroll_total;
        if (thumb_h < 20) thumb_h = 20;
        uint32_t max_scroll = win->scroll_total - win->scroll_visible;

        int32_t new_thumb_y = my - win->y - scroll_drag_offset;
        if (new_thumb_y < 0) new_thumb_y = 0;
        if (sh > thumb_h) {
            uint32_t new_pos = ((uint32_t)new_thumb_y * max_scroll) /
                               (sh - thumb_h);
            if (new_pos > max_scroll) new_pos = max_scroll;
            win->scroll_pos = new_pos;
            if (win->on_scroll) win->on_scroll(win, (int32_t)win->scroll_pos);
            global_dirty = true;
        }
        return;
    }

    if (!left_down) {
        dragging_win = NULL;
        scrollbar_dragging = false;
        scroll_drag_win = NULL;
    }

    /* Pencere surukle */
    if (dragging_win && left_down) {
        if (dragging_win->flags & WIN_FLAG_MAXIMIZED) {
            /* Maximize iken surukleme: once restore et */
            wm_restore_window(dragging_win);
            drag_offset_x = (int32_t)dragging_win->width / 2;
            drag_offset_y = -(int32_t)TITLEBAR_HEIGHT / 2;
        }
        int32_t nx = mx - drag_offset_x;
        int32_t ny = my - drag_offset_y;
        dragging_win->x = nx;
        dragging_win->y = ny;
        clamp_window(dragging_win);
        global_dirty = true;
        return;
    }

    if (!left_pressed && !right_pressed) return;

    for (int32_t i = (int32_t)win_count - 1; i >= 0; i--) {
        window_t* win = &windows[win_order[i]];
        if (!(win->flags & WIN_FLAG_VISIBLE)) continue;
        if (win->flags & WIN_FLAG_MINIMIZED) continue;

        int32_t wx = win->x;
        int32_t wy = win->y - (int32_t)TITLEBAR_HEIGHT;
        int32_t wr = wx + (int32_t)win->width;
        int32_t wb = win->y + (int32_t)win->height;

        if (mx >= wx && mx < wr && my >= wy && my < wb) {
            bring_to_front((uint32_t)i);

            /* Sag tik */
            if (right_pressed && my >= win->y && win->on_rightclick) {
                win->on_rightclick(win, mx - win->x, my - win->y);
                return;
            }

            if (!left_pressed) return;

            /* Titlebar butonlari kontrol */
            /* Kapatma (X) — en sagda */
            if (win->flags & WIN_FLAG_CLOSABLE) {
                int32_t cbx = wx + (int32_t)win->width - TITLE_BTN_SIZE - 4;
                int32_t cby = wy + 4;
                if (mx >= cbx && mx < cbx + (int32_t)TITLE_BTN_SIZE &&
                    my >= cby && my < cby + (int32_t)TITLE_BTN_SIZE) {
                    wm_close_window(win);
                    dragging_win = NULL;
                    return;
                }
            }

            /* Maximize/Restore — ortada */
            {
                int32_t mbx = wx + (int32_t)win->width -
                              (TITLE_BTN_SIZE + TITLE_BTN_GAP) * 2 - 4;
                int32_t mby = wy + 4;
                if (mx >= mbx && mx < mbx + (int32_t)TITLE_BTN_SIZE &&
                    my >= mby && my < mby + (int32_t)TITLE_BTN_SIZE) {
                    if (win->flags & WIN_FLAG_MAXIMIZED)
                        wm_restore_window(win);
                    else
                        wm_maximize_window(win);
                    return;
                }
            }

            /* Minimize (_) — en solda */
            {
                int32_t nbx = wx + (int32_t)win->width -
                              (TITLE_BTN_SIZE + TITLE_BTN_GAP) * 3 - 4;
                int32_t nby = wy + 4;
                if (mx >= nbx && mx < nbx + (int32_t)TITLE_BTN_SIZE &&
                    my >= nby && my < nby + (int32_t)TITLE_BTN_SIZE) {
                    wm_minimize_window(win);
                    return;
                }
            }

            /* Client area tiklama */
            if (my >= win->y) {
                /* Scrollbar mi? */
                if (handle_scrollbar_click(win, mx, my))
                    return;

                if (win->on_click)
                    win->on_click(win, mx - win->x, my - win->y);
            }

            /* Titlebar surukleme */
            if (my >= wy && my < win->y) {
                dragging_win = win;
                drag_offset_x = mx - win->x;
                drag_offset_y = my - win->y;
            }
            return;
        }
    }
}

/* ---- Scrollbar API ---- */

void wm_setup_scrollbar(window_t* win, uint32_t total_h, uint32_t visible_h)
{
    if (!win) return;
    win->flags |= WIN_FLAG_HAS_SCROLL;
    win->scroll_total = total_h;
    win->scroll_visible = visible_h;
    if (win->scroll_pos > 0 && win->scroll_total > win->scroll_visible) {
        uint32_t max_s = win->scroll_total - win->scroll_visible;
        if (win->scroll_pos > max_s) win->scroll_pos = max_s;
    }
}

void wm_scroll_up(window_t* win, uint32_t amount)
{
    if (!win || win->scroll_pos == 0) return;
    if (amount > win->scroll_pos)
        win->scroll_pos = 0;
    else
        win->scroll_pos -= amount;
    if (win->on_scroll) win->on_scroll(win, (int32_t)win->scroll_pos);
    global_dirty = true;
}

void wm_scroll_down(window_t* win, uint32_t amount)
{
    if (!win) return;
    if (win->scroll_total <= win->scroll_visible) return;
    uint32_t max_s = win->scroll_total - win->scroll_visible;
    win->scroll_pos += amount;
    if (win->scroll_pos > max_s) win->scroll_pos = max_s;
    if (win->on_scroll) win->on_scroll(win, (int32_t)win->scroll_pos);
    global_dirty = true;
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
