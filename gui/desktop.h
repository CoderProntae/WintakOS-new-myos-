#ifndef WINTAKOS_DESKTOP_H
#define WINTAKOS_DESKTOP_H

#include "../include/types.h"

#define TASKBAR_HEIGHT  32

void desktop_init(void);
void desktop_draw(void);
void desktop_draw_taskbar(uint32_t ticks, uint32_t frequency);
void desktop_update(void);

#endif
