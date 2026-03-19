#ifndef WINTAKOS_DESKTOP_H
#define WINTAKOS_DESKTOP_H

#include "../include/types.h"

#define TASKBAR_HEIGHT  32

void desktop_init(void);
void desktop_update(void);
void desktop_set_busy(bool b);
bool desktop_start_menu_open(void);
int  desktop_start_menu_click(int32_t mx, int32_t my);

#endif
