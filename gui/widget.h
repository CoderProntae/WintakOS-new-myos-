#ifndef WINTAKOS_WIDGET_H
#define WINTAKOS_WIDGET_H

#include "../include/types.h"
#include "window.h"

void widget_draw_label(window_t* win, uint32_t rx, uint32_t ry,
                       const char* text, uint32_t color);

void widget_draw_button(window_t* win, uint32_t rx, uint32_t ry,
                        uint32_t w, uint32_t h, const char* text,
                        uint32_t bg, uint32_t fg);

#endif
