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

/* Text input widget */
void widget_draw_textbox(window_t* win, uint32_t rx, uint32_t ry,
                         uint32_t w, const char* text, uint32_t cursor_pos,
                         bool focused);

/* Radio button */
void widget_draw_radio(window_t* win, uint32_t rx, uint32_t ry,
                       const char* label, bool selected, uint32_t color);

/* Progress bar */
void widget_draw_progress(window_t* win, uint32_t rx, uint32_t ry,
                          uint32_t w, uint32_t h, uint32_t percent,
                          uint32_t fg, uint32_t bg);

#endif
