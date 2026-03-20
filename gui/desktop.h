#ifndef WINTAKOS_DESKTOP_H
#define WINTAKOS_DESKTOP_H

#include "../include/types.h"

#define TASKBAR_HEIGHT  32

/* Cursor modlari */
#define CURSOR_NORMAL  0
#define CURSOR_BUSY    1
#define CURSOR_DRAG    2

void desktop_init(void);
void desktop_update(void);
void desktop_apply_config(void);
bool desktop_start_menu_open(void);
int  desktop_start_menu_click(int32_t mx, int32_t my);

/* Cursor modu */
void    desktop_set_cursor(uint8_t mode);
uint8_t desktop_get_cursor(void);

/* Masaustu ikon */
int desktop_icon_click(int32_t mx, int32_t my);
int desktop_icon_dblclick(int32_t mx, int32_t my);

#endif
