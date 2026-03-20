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
void desktop_set_cursor(uint8_t mode);
uint8_t desktop_get_cursor(void);

/* Masaustu ikon */
int desktop_icon_click(int32_t mx, int32_t my);
int desktop_icon_dblclick(int32_t mx, int32_t my);

/* Cozunurluk yonetimi */
void     desktop_request_resolution(uint32_t w, uint32_t h);
void     desktop_confirm_resolution(void);
void     desktop_revert_resolution(void);
uint32_t desktop_get_screen_w(void);
uint32_t desktop_get_screen_h(void);
uint32_t desktop_get_max_w(void);
uint32_t desktop_get_max_h(void);
bool     desktop_is_confirming(void);

#endif
