#ifndef WINTAKOS_FILEMANAGER_H
#define WINTAKOS_FILEMANAGER_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    int32_t   selected;
    int32_t   scroll;
} filemanager_t;

filemanager_t* filemanager_create(int32_t x, int32_t y);

#endif
