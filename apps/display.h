#ifndef WINTAKOS_DISPLAY_H
#define WINTAKOS_DISPLAY_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    uint32_t  selected;
} display_app_t;

display_app_t* display_create(int32_t x, int32_t y);

#endif
