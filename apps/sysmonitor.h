#ifndef WINTAKOS_SYSMONITOR_H
#define WINTAKOS_SYSMONITOR_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    uint32_t  last_tick;
} sysmonitor_t;

sysmonitor_t* sysmonitor_create(int32_t x, int32_t y);

#endif
