#ifndef WINTAKOS_DESKTOP_H
#define WINTAKOS_DESKTOP_H

#include "../include/types.h"

#define TASKBAR_HEIGHT  32

typedef void (*content_draw_fn)(void);

void desktop_init(void);
void desktop_set_content_drawer(content_draw_fn fn);
void desktop_update(void);

#endif
