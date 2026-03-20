#ifndef WINTAKOS_NETWORK_H
#define WINTAKOS_NETWORK_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
} network_app_t;

network_app_t* network_create(int32_t x, int32_t y);

#endif
