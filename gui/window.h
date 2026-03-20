#ifndef WINTAKOS_WINDOW_H
#define WINTAKOS_WINDOW_H

#include "../include/types.h"

#define MAX_WINDOWS       16
#define TITLEBAR_HEIGHT   24
#define BORDER_WIDTH      2
#define CLOSE_BTN_SIZE    16
#define TITLE_BTN_SIZE    16
#define TITLE_BTN_GAP     2

/* Pencere bayraklari */
#define WIN_FLAG_VISIBLE    0x01
#define WIN_FLAG_MINIMIZED  0x02
#define WIN_FLAG_CLOSABLE   0x04
#define WIN_FLAG_MAXIMIZED  0x08
#define WIN_FLAG_HAS_SCROLL 0x10

typedef struct window window_t;

typedef void (*win_draw_fn)(window_t* win);
typedef void (*win_click_fn)(window_t* win, int32_t rx, int32_t ry);
typedef void (*win_scroll_fn)(window_t* win, int32_t scroll_pos);
typedef void (*win_rightclick_fn)(window_t* win, int32_t rx, int32_t ry);

struct window {
    int32_t  x, y;
    uint32_t width, height;
    char     title[64];
    uint32_t bg_color;
    uint8_t  flags;
    uint8_t  id;
    bool     active;

    /* Callbacks */
    win_draw_fn       on_draw;
    win_click_fn      on_click;
    win_scroll_fn     on_scroll;
    win_rightclick_fn on_rightclick;

    /* Maximize oncesi konum */
    int32_t  saved_x, saved_y;
    uint32_t saved_w, saved_h;

    /* Scrollbar durumu */
    uint32_t scroll_total;   /* toplam icerik yuksekligi */
    uint32_t scroll_visible; /* gorunen alan yuksekligi */
    uint32_t scroll_pos;     /* mevcut kaydirma pozisyonu */
};

void      wm_init(void);
window_t* wm_create_window(int32_t x, int32_t y, uint32_t w, uint32_t h,
                            const char* title, uint32_t bg_color);
void      wm_close_window(window_t* win);
void      wm_minimize_window(window_t* win);
void      wm_maximize_window(window_t* win);
void      wm_restore_window(window_t* win);
void      wm_draw_all(void);
void      wm_handle_mouse(int32_t mx, int32_t my, uint8_t buttons);
window_t* wm_get_window(uint32_t index);
uint32_t  wm_get_count(void);
bool      wm_is_dirty(void);
void      wm_clear_dirty(void);
void      wm_set_dirty(void);

/* Scrollbar */
void wm_setup_scrollbar(window_t* win, uint32_t total_h, uint32_t visible_h);
void wm_scroll_up(window_t* win, uint32_t amount);
void wm_scroll_down(window_t* win, uint32_t amount);

#endif
