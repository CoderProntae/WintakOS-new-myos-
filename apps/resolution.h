#ifndef WINTAKOS_RESOLUTION_H
#define WINTAKOS_RESOLUTION_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    window_t* confirm_win;
    int       selected;
    bool      confirming;
    uint32_t  confirm_start;
    uint32_t  old_w, old_h;
    char      error_msg[48];
} resolution_app_t;

resolution_app_t* resolution_create(int32_t x, int32_t y);
void resolution_update(void);

#endif
