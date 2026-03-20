#ifndef WINTAKOS_WIDGET_H
#define WINTAKOS_WIDGET_H

#include "../include/types.h"
#include "window.h"

void widget_draw_label(window_t* win, uint32_t rx, uint32_t ry,
                       const char* text, uint32_t color);

void widget_draw_button(window_t* win, uint32_t rx, uint32_t ry,
                        uint32_t w, uint32_t h, const char* text,
                        uint32_t bg, uint32_t fg);

bool widget_button_hit(window_t* win, uint32_t rx, uint32_t ry,
                       uint32_t w, uint32_t h,
                       int32_t click_rx, int32_t click_ry);

void widget_draw_textbox(window_t* win, uint32_t rx, uint32_t ry,
                         uint32_t w, const char* text, uint32_t cursor_pos,
                         bool focused);

void widget_draw_radio(window_t* win, uint32_t rx, uint32_t ry,
                       const char* label, bool selected, uint32_t color);

void widget_draw_progress(window_t* win, uint32_t rx, uint32_t ry,
                          uint32_t w, uint32_t h, uint32_t percent,
                          uint32_t fg, uint32_t bg);

/* Yeni widgetlar */
void widget_draw_checkbox(window_t* win, uint32_t rx, uint32_t ry,
                          const char* label, bool checked, uint32_t color);

/* Context menu — ekrana dogrudan cizer */
#define CTX_MENU_MAX_ITEMS 8

typedef struct {
    int32_t  x, y;
    uint32_t width;
    uint32_t item_count;
    char     items[CTX_MENU_MAX_ITEMS][32];
    bool     visible;
} context_menu_t;

void ctx_menu_init(context_menu_t* menu);
void ctx_menu_add(context_menu_t* menu, const char* label);
void ctx_menu_show(context_menu_t* menu, int32_t x, int32_t y);
void ctx_menu_hide(context_menu_t* menu);
void ctx_menu_draw(context_menu_t* menu);
int  ctx_menu_click(context_menu_t* menu, int32_t mx, int32_t my);

#endif
